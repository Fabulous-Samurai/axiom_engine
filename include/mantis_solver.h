/**
 * @file mantis_solver.h
 * @brief Mantis A* Solver with hardware-accelerated heuristic dispatch
 *
 * Zero-allocation A* implementation using thread_local scratch buffers
 * and stack-allocated fixed-capacity containers. The Heuristic() call
 * dispatches to MantisHeuristic::evaluate_f32 on the hot path.
 *
 * Latency Budget: < 5ns per node evaluation
 * Memory Policy:  Zero heap allocation in the hot loop
 */

#pragma once

#ifndef MANTIS_SOLVER_H
#define MANTIS_SOLVER_H

#include "mantis_heuristic.h"
#include <array>
#include <cstdint>
#include <cstddef>
#include <limits>

namespace AXIOM {
namespace Mantis {

// ============================================================================
// A* Node — compact, cache-friendly layout
// ============================================================================
inline constexpr size_t kMaxNodes = 4096;  // Fixed-capacity for zero-allocation

struct alignas(64) AStarNode {
    uint32_t id         = 0;
    uint32_t parent_id  = UINT32_MAX;  // Sentinel: no parent
    float    g_cost     = std::numeric_limits<float>::max();  // Cost from start
    float    h_cost     = 0.0f;        // Heuristic estimate to goal
    float    f_cost     = std::numeric_limits<float>::max();  // g + h
    bool     in_closed  = false;

    NodeFeatureVecF32 features{};      // Feature vector for heuristic eval
};

// ============================================================================
// Fixed-Capacity Min-Heap (no allocation)
// ============================================================================
class FixedMinHeap {
public:
    AXIOM_FORCE_INLINE bool empty() const noexcept { return size_ == 0; }
    AXIOM_FORCE_INLINE size_t size() const noexcept { return size_; }

    AXIOM_FORCE_INLINE bool push(uint32_t node_id, float f_cost) noexcept {
        if (size_ >= kMaxNodes) [[unlikely]] return false;

        entries_[size_] = {node_id, f_cost};
        sift_up(size_);
        ++size_;
        return true;
    }

    AXIOM_FORCE_INLINE uint32_t pop() noexcept {
        const uint32_t top_id = entries_[0].node_id;
        --size_;
        entries_[0] = entries_[size_];
        if (size_ > 0) sift_down(0);
        return top_id;
    }

private:
    struct Entry {
        uint32_t node_id = 0;
        float    f_cost  = 0.0f;
    };

    std::array<Entry, kMaxNodes> entries_{};
    size_t size_ = 0;

    AXIOM_FORCE_INLINE void sift_up(size_t idx) noexcept {
        while (idx > 0) {
            const size_t parent = (idx - 1) / 2;
            if (entries_[idx].f_cost < entries_[parent].f_cost) {
                std::swap(entries_[idx], entries_[parent]);
                idx = parent;
            } else break;
        }
    }

    AXIOM_FORCE_INLINE void sift_down(size_t idx) noexcept {
        while (true) {
            size_t smallest = idx;
            const size_t left  = 2 * idx + 1;
            const size_t right = 2 * idx + 2;

            if (left < size_ && entries_[left].f_cost < entries_[smallest].f_cost)
                smallest = left;
            if (right < size_ && entries_[right].f_cost < entries_[smallest].f_cost)
                smallest = right;

            if (smallest != idx) {
                std::swap(entries_[idx], entries_[smallest]);
                idx = smallest;
            } else break;
        }
    }
};

// ============================================================================
// AStarSolver — zero-allocation A* with Mantis heuristic
// ============================================================================
class AStarSolver {
public:
    struct SearchResult {
        bool     found     = false;
        uint32_t goal_id   = UINT32_MAX;
        float    total_cost = 0.0f;
        uint32_t nodes_evaluated = 0;
    };

    /**
     * @brief Set the target profile that the heuristic matches against.
     * Must be called before solve().
     */
    void set_target_profile(const TargetProfileF32& profile) noexcept {
        target_profile_ = profile;
    }

    /**
     * @brief Set the Dog threshold for conditional normalization.
     */
    void set_dog_threshold(float threshold) noexcept {
        dog_threshold_ = threshold;
    }

    /**
     * @brief Evaluate the heuristic for a single node.
     * This is the HOT PATH — must be < 5ns.
     */
    AXIOM_FORCE_INLINE float Heuristic(const AStarNode& node) const noexcept {
        return MantisHeuristic::evaluate_f32(
            node.features, target_profile_, dog_threshold_);
    }

    /**
     * @brief Run A* search over the node array.
     * @param nodes     Pointer to node storage (caller-owned)
     * @param num_nodes Number of nodes in the graph
     * @param start_id  Starting node ID
     * @param goal_id   Goal node ID
     * @param adjacency Function that fills neighbor IDs for a node
     *
     * Uses thread_local scratch buffers internally — fully reentrant
     * across threads but NOT recursion-safe within the same thread.
     */
    template<typename AdjacencyFn>
    SearchResult solve(
        AStarNode* nodes,
        uint32_t num_nodes,
        uint32_t start_id,
        uint32_t goal_id,
        AdjacencyFn&& get_neighbors) noexcept
    {
        if (num_nodes == 0 || num_nodes > kMaxNodes) [[unlikely]] {
            return {};
        }

        // Reset node states
        for (uint32_t i = 0; i < num_nodes; ++i) {
            nodes[i].g_cost = std::numeric_limits<float>::max();
            nodes[i].f_cost = std::numeric_limits<float>::max();
            nodes[i].parent_id = UINT32_MAX;
            nodes[i].in_closed = false;
        }

        // thread_local scratch: neighbor buffer
        thread_local std::array<uint32_t, 64> neighbor_buf{};
        thread_local FixedMinHeap open_set{};

        // Clear open set
        open_set = FixedMinHeap{};

        // Initialize start node
        nodes[start_id].g_cost = 0.0f;
        nodes[start_id].h_cost = Heuristic(nodes[start_id]);
        nodes[start_id].f_cost = nodes[start_id].h_cost;
        open_set.push(start_id, nodes[start_id].f_cost);

        SearchResult result{};

        while (!open_set.empty()) {
            const uint32_t current = open_set.pop();
            ++result.nodes_evaluated;

            if (current == goal_id) {
                result.found = true;
                result.goal_id = goal_id;
                result.total_cost = nodes[goal_id].g_cost;
                return result;
            }

            if (nodes[current].in_closed) continue;
            nodes[current].in_closed = true;

            // Get neighbors — caller fills neighbor_buf, returns count
            const uint32_t n_count = get_neighbors(
                current, neighbor_buf.data(), neighbor_buf.size());

            for (uint32_t i = 0; i < n_count; ++i) {
                const uint32_t neighbor = neighbor_buf[i];
                if (neighbor >= num_nodes || nodes[neighbor].in_closed) continue;

                const float tentative_g = nodes[current].g_cost + 1.0f; // Unit cost
                if (tentative_g < nodes[neighbor].g_cost) {
                    nodes[neighbor].parent_id = current;
                    nodes[neighbor].g_cost = tentative_g;
                    nodes[neighbor].h_cost = Heuristic(nodes[neighbor]);
                    nodes[neighbor].f_cost = tentative_g + nodes[neighbor].h_cost;
                    open_set.push(neighbor, nodes[neighbor].f_cost);
                }
            }
        }

        return result; // Not found
    }

private:
    TargetProfileF32 target_profile_{};
    float dog_threshold_ = kDogThreshold;
};

} // namespace Mantis
} // namespace AXIOM

#endif // MANTIS_SOLVER_H
