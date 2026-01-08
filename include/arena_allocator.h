/**
 * @file arena_allocator.h
 * @brief AXIOM Engine v3.0 - High-Performance Arena Memory Allocator
 * 
 * Enterprise-grade memory management system:
 * - Zero-fragmentation arena allocation
 * - NUMA-aware memory pools
 * - Cache-line aligned allocations
 * - Memory-mapped file backing
 * - Real-time performance guarantees
 */

#pragma once

#include <memory>
#include <vector>
#include <unordered_map>
#include <mutex>
#include <atomic>
#include <cstddef>
#include <cstdint>
#include <type_traits>

#ifdef _WIN32
    #include <windows.h>
#else
    #include <sys/mman.h>
    #include <unistd.h>
#endif

#ifdef ENABLE_EIGEN
    #include <Eigen/Dense>
#endif

namespace AXIOM {

/**
 * @brief High-Performance Memory Arena
 * 
 * Zero-fragmentation allocator for scientific computing workloads
 */
class MemoryArena {
public:
    static constexpr size_t DEFAULT_ARENA_SIZE = 64 * 1024 * 1024;  // 64MB
    static constexpr size_t CACHE_LINE_SIZE = 64;
    static constexpr size_t PAGE_SIZE = 4096;
    
    struct ArenaStats {
        size_t total_size;
        size_t used_size;
        size_t free_size;
        size_t peak_usage;
        size_t allocation_count;
        size_t free_count;
        double fragmentation_ratio;
    };

private:
    void* memory_base_;
    size_t arena_size_;
    std::atomic<size_t> current_offset_;
    std::atomic<size_t> peak_usage_;
    std::atomic<size_t> allocation_count_;
    std::atomic<size_t> free_count_;
    
    // Free block management
    struct FreeBlock {
        size_t size;
        FreeBlock* next;
    };
    
    FreeBlock* free_list_head_;
    std::mutex free_list_mutex_;
    
    // Memory mapping for large allocations
    bool use_memory_mapping_;
    
#ifdef _WIN32
    HANDLE file_mapping_;
#else
    int backing_fd_;
#endif

public:
    explicit MemoryArena(size_t size = DEFAULT_ARENA_SIZE, bool use_mmap = true);
    ~MemoryArena();
    
    // Core allocation interface
    void* allocate(size_t size, size_t alignment = CACHE_LINE_SIZE);
    void deallocate(void* ptr, size_t size);
    
    // Typed allocation helpers
    template<typename T>
    T* allocate_array(size_t count) {
        size_t total_size = count * sizeof(T);
        void* ptr = allocate(total_size, alignof(T));
        return static_cast<T*>(ptr);
    }
    
    template<typename T, typename... Args>
    T* construct(Args&&... args) {
        void* ptr = allocate(sizeof(T), alignof(T));
        return new(ptr) T(std::forward<Args>(args)...);
    }
    
    template<typename T>
    void destroy(T* obj) {
        if (obj) {
            obj->~T();
            deallocate(obj, sizeof(T));
        }
    }
    
    // Arena management
    void reset();
    void trim();
    ArenaStats get_stats() const;
    
    // Memory layout optimization
    void* allocate_aligned_page(size_t size);
    void prefault_pages(void* ptr, size_t size);
    
    // NUMA awareness (Linux only)
#ifndef _WIN32
    bool set_numa_policy(int node);
    int get_numa_node() const;
#endif

private:
    bool setup_memory_mapping(size_t size);
    void cleanup_memory_mapping();
    void coalesce_free_blocks();
    size_t align_size(size_t size, size_t alignment) const;
    bool is_pointer_in_arena(void* ptr) const;
};

/**
 * @brief NUMA-Aware Memory Pool Manager
 * 
 * Manages multiple arenas across NUMA nodes for optimal performance
 */
class PoolManager {
public:
    enum class PoolType {
        SMALL_OBJECTS,    // < 1KB
        MEDIUM_OBJECTS,   // 1KB - 64KB  
        LARGE_OBJECTS,    // 64KB - 16MB
        HUGE_OBJECTS      // > 16MB
    };

private:
    struct PoolInfo {
        std::unique_ptr<MemoryArena> arena;
        PoolType type;
        int numa_node;
        std::mutex allocation_mutex;
        std::atomic<size_t> active_allocations;
        
        // Default constructor
        PoolInfo() : type(PoolType::SMALL_OBJECTS), numa_node(0), active_allocations(0) {}
        
        // Explicit move constructor since mutex is not moveable
        PoolInfo(PoolInfo&& other) noexcept 
            : arena(std::move(other.arena))
            , type(other.type)
            , numa_node(other.numa_node)
            , allocation_mutex()
            , active_allocations(other.active_allocations.load()) {}
    };
    
    std::vector<PoolInfo> pools_;
    std::unordered_map<void*, size_t> allocation_map_;
    std::mutex allocation_map_mutex_;
    
    // Thread-local pool caching
    thread_local static size_t preferred_pool_index_;
    
public:
    PoolManager();
    ~PoolManager();
    
    // Global allocation interface
    void* allocate(size_t size, size_t alignment = MemoryArena::CACHE_LINE_SIZE);
    void deallocate(void* ptr);
    
    // Pool management
    size_t add_pool(PoolType type, size_t arena_size = MemoryArena::DEFAULT_ARENA_SIZE, int numa_node = -1);
    void remove_pool(size_t pool_index);
    
    // Performance monitoring
    std::vector<MemoryArena::ArenaStats> get_all_stats() const;
    size_t get_total_allocated() const;
    
    // Optimization
    void optimize_pools();
    void balance_pools();
    
    // Global instance
    static PoolManager& instance();

private:
    PoolType classify_allocation(size_t size) const;
    size_t select_optimal_pool(PoolType type, int numa_node = -1);
    void update_allocation_map(void* ptr, size_t pool_index);
};

/**
 * @brief STL-Compatible Arena Allocator
 * 
 * Drop-in replacement for std::allocator using AXIOM memory pools
 */
template<typename T>
class ArenaAllocator {
public:
    using value_type = T;
    using pointer = T*;
    using const_pointer = const T*;
    using reference = T&;
    using const_reference = const T&;
    using size_type = std::size_t;
    using difference_type = std::ptrdiff_t;
    
    template<typename U>
    struct rebind {
        using other = ArenaAllocator<U>;
    };

private:
    MemoryArena* arena_;

public:
    ArenaAllocator() : arena_(nullptr) {}
    explicit ArenaAllocator(MemoryArena* arena) : arena_(arena) {}
    
    template<typename U>
    ArenaAllocator(const ArenaAllocator<U>& other) : arena_(other.get_arena()) {}
    
    pointer allocate(size_type n) {
        if (arena_) {
            return arena_->allocate_array<T>(n);
        } else {
            return static_cast<pointer>(PoolManager::instance().allocate(n * sizeof(T), alignof(T)));
        }
    }
    
    void deallocate(pointer p, size_type n) {
        if (arena_) {
            arena_->deallocate(p, n * sizeof(T));
        } else {
            PoolManager::instance().deallocate(p);
        }
    }
    
    template<typename U, typename... Args>
    void construct(U* p, Args&&... args) {
        new(p) U(std::forward<Args>(args)...);
    }
    
    template<typename U>
    void destroy(U* p) {
        p->~U();
    }
    
    MemoryArena* get_arena() const { return arena_; }
    
    bool operator==(const ArenaAllocator& other) const {
        return arena_ == other.arena_;
    }
    
    bool operator!=(const ArenaAllocator& other) const {
        return !(*this == other);
    }
};

/**
 * @brief High-Performance Vector with Arena Allocation
 * 
 * Optimized vector for numerical computations
 */
template<typename T>
using ArenaVector = std::vector<T, ArenaAllocator<T>>;

/**
 * @brief Matrix Memory Layout Optimizer
 * 
 * Cache-friendly memory layout for dense matrices
 */
template<typename T>
class OptimizedMatrix {
private:
    ArenaVector<T> data_;
    size_t rows_;
    size_t cols_;
    size_t row_stride_;  // For cache alignment

public:
    OptimizedMatrix(size_t rows, size_t cols, MemoryArena* arena = nullptr)
        : data_(ArenaAllocator<T>(arena))
        , rows_(rows)
        , cols_(cols)
    {
        // Align row stride to cache line boundaries
        row_stride_ = (cols_ * sizeof(T) + MemoryArena::CACHE_LINE_SIZE - 1) / MemoryArena::CACHE_LINE_SIZE;
        row_stride_ *= MemoryArena::CACHE_LINE_SIZE / sizeof(T);
        
        data_.resize(rows_ * row_stride_);
    }
    
    T& operator()(size_t row, size_t col) {
        return data_[row * row_stride_ + col];
    }
    
    const T& operator()(size_t row, size_t col) const {
        return data_[row * row_stride_ + col];
    }
    
    size_t rows() const { return rows_; }
    size_t cols() const { return cols_; }
    
    // Direct memory access for BLAS operations
    T* data() { return data_.data(); }
    const T* data() const { return data_.data(); }
    size_t stride() const { return row_stride_; }
};

#ifdef ENABLE_EIGEN
/**
 * @brief Eigen Integration Helper
 * 
 * Custom allocator for Eigen matrices using AXIOM memory pools
 */
namespace EigenIntegration {
    template<typename Scalar, int Options = 0>
    using Matrix = Eigen::Matrix<Scalar, Eigen::Dynamic, Eigen::Dynamic, Options>;
    
    template<typename Scalar>
    Matrix<Scalar> create_matrix(size_t rows, size_t cols, MemoryArena* arena = nullptr);
    
    template<typename Scalar>
    class ArenaAlignedAllocator {
    public:
        using value_type = Scalar;
        MemoryArena* arena_;
        
        ArenaAlignedAllocator(MemoryArena* arena = nullptr) : arena_(arena) {}
        
        Scalar* allocate(size_t num_elements) {
            if (arena_) {
                return arena_->allocate_array<Scalar>(num_elements);
            } else {
                return static_cast<Scalar*>(PoolManager::instance().allocate(
                    num_elements * sizeof(Scalar), 32));  // 32-byte alignment for SIMD
            }
        }
        
        void deallocate(Scalar* ptr, size_t num_elements) {
            if (arena_) {
                arena_->deallocate(ptr, num_elements * sizeof(Scalar));
            } else {
                PoolManager::instance().deallocate(ptr);
            }
        }
    };
}
#endif // ENABLE_EIGEN

/**
 * @brief Memory Performance Profiler
 * 
 * Real-time memory performance monitoring and optimization
 */
class MemoryProfiler {
public:
    struct AllocationProfile {
        size_t size;
        size_t alignment;
        std::chrono::high_resolution_clock::time_point timestamp;
        void* address;
        size_t pool_index;
    };
    
    struct PerformanceMetrics {
        double avg_allocation_time_ns;
        double avg_deallocation_time_ns;
        size_t cache_hits;
        size_t cache_misses;
        double memory_efficiency;
        size_t peak_memory_usage;
    };

private:
    std::vector<AllocationProfile> allocation_history_;
    mutable std::mutex history_mutex_;
    std::atomic<bool> profiling_enabled_;

public:
    MemoryProfiler();
    
    void enable_profiling(bool enable = true);
    bool is_profiling_enabled() const;
    
    void record_allocation(void* ptr, size_t size, size_t alignment, size_t pool_index);
    void record_deallocation(void* ptr);
    
    PerformanceMetrics get_metrics() const;
    std::vector<AllocationProfile> get_recent_allocations(size_t count = 100) const;
    
    // Analysis and optimization suggestions
    std::vector<std::string> analyze_patterns() const;
    std::vector<std::string> get_optimization_suggestions() const;
    
    // Global profiler instance
    static MemoryProfiler& instance();
};

} // namespace AXIOM