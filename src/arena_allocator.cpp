/**
 * @file arena_allocator.cpp  
 * @brief AXIOM Engine v3.0 - Arena Allocator Implementation
 * 
 * Ultra-high performance memory management system
 */

#include "arena_allocator.h"
#include <algorithm>
#include <cstring>
#include <cassert>
#include <iostream>

#ifdef _WIN32
    #include <memoryapi.h>
#else
    #include <sys/mman.h>
    #include <unistd.h>
    #include <numa.h>
    #include <numaif.h>
#endif

namespace AXIOM {

// ============================================================================
// MemoryArena Implementation
// ============================================================================

MemoryArena::MemoryArena(size_t size, bool use_mmap)
    : arena_size_(size)
    , current_offset_(0)
    , peak_usage_(0)
    , allocation_count_(0)
    , free_count_(0)
    , free_list_head_(nullptr)
    , use_memory_mapping_(use_mmap)
#ifdef _WIN32
    , file_mapping_(nullptr)
#else
    , backing_fd_(-1)
#endif
{
    if (use_memory_mapping_) {
        if (!setup_memory_mapping(size)) {
            // Fallback to regular allocation (Windows uses _aligned_malloc)
#ifdef _WIN32
            memory_base_ = _aligned_malloc(arena_size_, PAGE_SIZE);
#else
            memory_base_ = ::aligned_alloc(PAGE_SIZE, arena_size_);
#endif
            if (!memory_base_) {
                throw std::bad_alloc();
            }
        }
    } else {
#ifdef _WIN32
        memory_base_ = _aligned_malloc(arena_size_, PAGE_SIZE);
#else
        memory_base_ = ::aligned_alloc(PAGE_SIZE, arena_size_);
#endif
        if (!memory_base_) {
            throw std::bad_alloc();
        }
    }
    
    // Pre-fault pages for better real-time performance
    prefault_pages(memory_base_, arena_size_);
}

MemoryArena::~MemoryArena() {
    if (use_memory_mapping_) {
        cleanup_memory_mapping();
    } else {
        std::free(memory_base_);
    }
}

void* MemoryArena::allocate(size_t size, size_t alignment) {
    if (size == 0) {
        return nullptr;
    }
    
    size_t aligned_size = align_size(size, alignment);
    allocation_count_.fetch_add(1, std::memory_order_relaxed);
    
    // Try free list first for better memory reuse
    {
        std::lock_guard<std::mutex> lock(free_list_mutex_);
        FreeBlock** current = &free_list_head_;
        
        while (*current) {
            if ((*current)->size >= aligned_size) {
                FreeBlock* block = *current;
                *current = block->next;
                
                // Split block if it's significantly larger
                if (block->size > aligned_size + sizeof(FreeBlock) + 64) {
                    FreeBlock* new_block = reinterpret_cast<FreeBlock*>(
                        reinterpret_cast<char*>(block) + aligned_size);
                    new_block->size = block->size - aligned_size;
                    new_block->next = *current;
                    *current = new_block;
                }
                
                return block;
            }
            current = &(*current)->next;
        }
    }
    
    // Allocate from main arena
    size_t current_pos = current_offset_.fetch_add(aligned_size, std::memory_order_acq_rel);
    
    if (current_pos + aligned_size > arena_size_) {
        current_offset_.fetch_sub(aligned_size, std::memory_order_acq_rel);
        throw std::bad_alloc();
    }
    
    void* ptr = static_cast<char*>(memory_base_) + current_pos;
    
    // Update peak usage
    size_t new_usage = current_pos + aligned_size;
    size_t current_peak = peak_usage_.load(std::memory_order_relaxed);
    while (new_usage > current_peak && 
           !peak_usage_.compare_exchange_weak(current_peak, new_usage, 
                                            std::memory_order_relaxed)) {
        // Retry loop for atomic max update
    }
    
    return ptr;
}

void MemoryArena::deallocate(void* ptr, size_t size) {
    if (!ptr || !is_pointer_in_arena(ptr)) {
        return;
    }
    
    free_count_.fetch_add(1, std::memory_order_relaxed);
    
    // Add to free list
    size_t aligned_size = align_size(size, CACHE_LINE_SIZE);
    FreeBlock* block = static_cast<FreeBlock*>(ptr);
    block->size = aligned_size;
    
    {
        std::lock_guard<std::mutex> lock(free_list_mutex_);
        block->next = free_list_head_;
        free_list_head_ = block;
    }
    
    // Periodic coalescing
    if (free_count_.load(std::memory_order_relaxed) % 1000 == 0) {
        coalesce_free_blocks();
    }
}

bool MemoryArena::setup_memory_mapping(size_t size) {
#ifdef _WIN32
    file_mapping_ = CreateFileMappingA(
        INVALID_HANDLE_VALUE,
        nullptr,
        PAGE_READWRITE,
        static_cast<DWORD>(size >> 32),
        static_cast<DWORD>(size & 0xFFFFFFFF),
        nullptr
    );
    
    if (!file_mapping_) {
        return false;
    }
    
    memory_base_ = MapViewOfFile(
        file_mapping_,
        FILE_MAP_ALL_ACCESS,
        0, 0, size
    );
    
    return memory_base_ != nullptr;
#else
    memory_base_ = mmap(
        nullptr, size,
        PROT_READ | PROT_WRITE,
        MAP_PRIVATE | MAP_ANONYMOUS,
        -1, 0
    );
    
    if (memory_base_ == MAP_FAILED) {
        memory_base_ = nullptr;
        return false;
    }
    
    // Advise kernel about usage patterns
    madvise(memory_base_, size, MADV_WILLNEED);
    madvise(memory_base_, size, MADV_SEQUENTIAL);
    
    return true;
#endif
}

void MemoryArena::cleanup_memory_mapping() {
#ifdef _WIN32
    if (memory_base_) {
        UnmapViewOfFile(memory_base_);
        memory_base_ = nullptr;
    }
    if (file_mapping_) {
        CloseHandle(file_mapping_);
        file_mapping_ = nullptr;
    }
#else
    if (memory_base_) {
        munmap(memory_base_, arena_size_);
        memory_base_ = nullptr;
    }
#endif
}

void MemoryArena::prefault_pages(void* ptr, size_t size) {
    // Touch each page to pre-fault for real-time performance
    volatile char* touch_ptr = static_cast<volatile char*>(ptr);
    for (size_t i = 0; i < size; i += PAGE_SIZE) {
        touch_ptr[i] = 0;
    }
}

void MemoryArena::reset() {
    current_offset_.store(0, std::memory_order_release);
    
    {
        std::lock_guard<std::mutex> lock(free_list_mutex_);
        free_list_head_ = nullptr;
    }
    
    free_count_.store(0, std::memory_order_relaxed);
}

void MemoryArena::coalesce_free_blocks() {
    std::lock_guard<std::mutex> lock(free_list_mutex_);
    
    if (!free_list_head_) {
        return;
    }
    
    // Sort free blocks by address for coalescing
    std::vector<FreeBlock*> blocks;
    FreeBlock* current = free_list_head_;
    while (current) {
        blocks.push_back(current);
        current = current->next;
    }
    
    std::sort(blocks.begin(), blocks.end());
    
    // Coalesce adjacent blocks
    size_t write_index = 0;
    for (size_t read_index = 0; read_index < blocks.size(); ++read_index) {
        if (write_index > 0) {
            char* prev_end = reinterpret_cast<char*>(blocks[write_index - 1]) + 
                           blocks[write_index - 1]->size;
            char* current_start = reinterpret_cast<char*>(blocks[read_index]);
            
            if (prev_end == current_start) {
                // Coalesce with previous block
                blocks[write_index - 1]->size += blocks[read_index]->size;
                continue;
            }
        }
        
        blocks[write_index++] = blocks[read_index];
    }
    
    // Rebuild free list
    free_list_head_ = nullptr;
    for (size_t i = write_index; i > 0; --i) {
        blocks[i - 1]->next = free_list_head_;
        free_list_head_ = blocks[i - 1];
    }
}

size_t MemoryArena::align_size(size_t size, size_t alignment) const {
    return (size + alignment - 1) & ~(alignment - 1);
}

bool MemoryArena::is_pointer_in_arena(void* ptr) const {
    char* char_ptr = static_cast<char*>(ptr);
    char* arena_start = static_cast<char*>(memory_base_);
    char* arena_end = arena_start + arena_size_;
    
    return char_ptr >= arena_start && char_ptr < arena_end;
}

MemoryArena::ArenaStats MemoryArena::get_stats() const {
    ArenaStats stats;
    stats.total_size = arena_size_;
    stats.used_size = current_offset_.load(std::memory_order_acquire);
    stats.free_size = arena_size_ - stats.used_size;
    stats.peak_usage = peak_usage_.load(std::memory_order_relaxed);
    stats.allocation_count = allocation_count_.load(std::memory_order_relaxed);
    stats.free_count = free_count_.load(std::memory_order_relaxed);
    
    // Calculate fragmentation ratio
    size_t free_block_count = 0;
    size_t free_block_total = 0;
    {
        std::lock_guard<std::mutex> lock(const_cast<std::mutex&>(free_list_mutex_));
        FreeBlock* block = free_list_head_;
        while (block) {
            free_block_count++;
            free_block_total += block->size;
            block = block->next;
        }
    }
    
    stats.fragmentation_ratio = free_block_count > 0 ? 
        static_cast<double>(free_block_count) / (stats.used_size / 1024.0) : 0.0;
    
    return stats;
}

#ifndef _WIN32
bool MemoryArena::set_numa_policy(int node) {
    if (numa_available() < 0) {
        return false;
    }
    
    unsigned long node_mask = 1UL << node;
    return mbind(memory_base_, arena_size_, MPOL_BIND, &node_mask, 
                sizeof(node_mask) * 8, MPOL_MF_STRICT) == 0;
}

int MemoryArena::get_numa_node() const {
    if (numa_available() < 0) {
        return -1;
    }
    
    int mode;
    unsigned long* node_mask = nullptr;
    unsigned long max_nodes = numa_max_possible_node() + 1;
    
    if (get_mempolicy(&mode, node_mask, max_nodes, memory_base_, MPOL_F_ADDR) == 0) {
        // Find first set bit
        for (int i = 0; i < static_cast<int>(max_nodes); ++i) {
            if (node_mask && (node_mask[i / (sizeof(unsigned long) * 8)] & 
                             (1UL << (i % (sizeof(unsigned long) * 8))))) {
                return i;
            }
        }
    }
    
    return -1;
}
#endif

// ============================================================================
// PoolManager Implementation  
// ============================================================================

thread_local size_t PoolManager::preferred_pool_index_ = SIZE_MAX;

PoolManager::PoolManager() {
    // Initialize default pools
    add_pool(PoolType::SMALL_OBJECTS, 16 * 1024 * 1024);   // 16MB for small objects
    add_pool(PoolType::MEDIUM_OBJECTS, 64 * 1024 * 1024);  // 64MB for medium objects
    add_pool(PoolType::LARGE_OBJECTS, 256 * 1024 * 1024);  // 256MB for large objects
    add_pool(PoolType::HUGE_OBJECTS, 1024 * 1024 * 1024);  // 1GB for huge objects
}

PoolManager::~PoolManager() = default;

void* PoolManager::allocate(size_t size, size_t alignment) {
    PoolType pool_type = classify_allocation(size);
    size_t pool_index = select_optimal_pool(pool_type);
    
    if (pool_index >= pools_.size()) {
        throw std::bad_alloc();
    }
    
    PoolInfo& pool = pools_[pool_index];
    
    {
        std::lock_guard<std::mutex> lock(pool.allocation_mutex);
        void* ptr = pool.arena->allocate(size, alignment);
        
        if (ptr) {
            pool.active_allocations.fetch_add(1, std::memory_order_relaxed);
            update_allocation_map(ptr, pool_index);
            return ptr;
        }
    }
    
    throw std::bad_alloc();
}

void PoolManager::deallocate(void* ptr) {
    if (!ptr) {
        return;
    }
    
    size_t pool_index;
    {
        std::lock_guard<std::mutex> lock(allocation_map_mutex_);
        auto it = allocation_map_.find(ptr);
        if (it == allocation_map_.end()) {
            return; // Not allocated by this pool manager
        }
        pool_index = it->second;
        allocation_map_.erase(it);
    }
    
    if (pool_index < pools_.size()) {
        PoolInfo& pool = pools_[pool_index];
        // Note: We don't know the original size, so we can't call deallocate
        // In a production system, we'd store size information
        pool.active_allocations.fetch_sub(1, std::memory_order_relaxed);
    }
}

size_t PoolManager::add_pool(PoolType type, size_t arena_size, int numa_node) {
    PoolInfo pool;
    pool.type = type;
    pool.numa_node = numa_node;
    pool.arena = std::make_unique<MemoryArena>(arena_size, true);
    pool.active_allocations.store(0);
    
#ifndef _WIN32
    if (numa_node >= 0) {
        pool.arena->set_numa_policy(numa_node);
    }
#endif
    
    pools_.push_back(std::move(pool));
    return pools_.size() - 1;
}

PoolManager::PoolType PoolManager::classify_allocation(size_t size) const {
    if (size <= 1024) {
        return PoolType::SMALL_OBJECTS;
    } else if (size <= 64 * 1024) {
        return PoolType::MEDIUM_OBJECTS;
    } else if (size <= 16 * 1024 * 1024) {
        return PoolType::LARGE_OBJECTS;
    } else {
        return PoolType::HUGE_OBJECTS;
    }
}

size_t PoolManager::select_optimal_pool(PoolType type, int numa_node) {
    // Try preferred pool first (thread-local cache)
    if (preferred_pool_index_ < pools_.size() && 
        pools_[preferred_pool_index_].type == type) {
        return preferred_pool_index_;
    }
    
    // Find best matching pool
    for (size_t i = 0; i < pools_.size(); ++i) {
        if (pools_[i].type == type) {
            if (numa_node < 0 || pools_[i].numa_node == numa_node || 
                pools_[i].numa_node < 0) {
                preferred_pool_index_ = i;
                return i;
            }
        }
    }
    
    // Fallback to any compatible pool
    for (size_t i = 0; i < pools_.size(); ++i) {
        if (pools_[i].type == type) {
            preferred_pool_index_ = i;
            return i;
        }
    }
    
    return SIZE_MAX;
}

void PoolManager::update_allocation_map(void* ptr, size_t pool_index) {
    std::lock_guard<std::mutex> lock(allocation_map_mutex_);
    allocation_map_[ptr] = pool_index;
}

std::vector<MemoryArena::ArenaStats> PoolManager::get_all_stats() const {
    std::vector<MemoryArena::ArenaStats> all_stats;
    all_stats.reserve(pools_.size());
    
    for (const auto& pool : pools_) {
        all_stats.push_back(pool.arena->get_stats());
    }
    
    return all_stats;
}

PoolManager& PoolManager::instance() {
    static PoolManager manager;
    return manager;
}

// ============================================================================
// MemoryProfiler Implementation
// ============================================================================

MemoryProfiler::MemoryProfiler() : profiling_enabled_(false) {
}

void MemoryProfiler::enable_profiling(bool enable) {
    profiling_enabled_.store(enable, std::memory_order_release);
}

bool MemoryProfiler::is_profiling_enabled() const {
    return profiling_enabled_.load(std::memory_order_acquire);
}

void MemoryProfiler::record_allocation(void* ptr, size_t size, size_t alignment, size_t pool_index) {
    if (!is_profiling_enabled()) {
        return;
    }
    
    AllocationProfile profile;
    profile.address = ptr;
    profile.size = size;
    profile.alignment = alignment;
    profile.pool_index = pool_index;
    profile.timestamp = std::chrono::high_resolution_clock::now();
    
    {
        std::lock_guard<std::mutex> lock(history_mutex_);
        allocation_history_.push_back(profile);
        
        // Limit history size
        if (allocation_history_.size() > 10000) {
            allocation_history_.erase(allocation_history_.begin(), 
                                    allocation_history_.begin() + 1000);
        }
    }
}

MemoryProfiler::PerformanceMetrics MemoryProfiler::get_metrics() const {
    std::lock_guard<std::mutex> lock(history_mutex_);
    
    PerformanceMetrics metrics{};
    
    if (allocation_history_.empty()) {
        return metrics;
    }
    
    // Calculate timing metrics (simplified)
    auto recent_start = allocation_history_.size() > 1000 ? 
                       allocation_history_.end() - 1000 : allocation_history_.begin();
    
    double total_allocation_time = 0.0;
    size_t allocation_count = 0;
    
    for (auto it = recent_start; it != allocation_history_.end(); ++it) {
        // Approximate allocation time based on size
        total_allocation_time += it->size / 1000.0; // Simplified
        allocation_count++;
    }
    
    if (allocation_count > 0) {
        metrics.avg_allocation_time_ns = total_allocation_time / allocation_count;
    }
    
    // Calculate peak usage (simplified)
    metrics.peak_memory_usage = 0;
    for (const auto& profile : allocation_history_) {
        if (profile.size > metrics.peak_memory_usage) {
            metrics.peak_memory_usage = profile.size;
        }
    }
    
    return metrics;
}

MemoryProfiler& MemoryProfiler::instance() {
    static MemoryProfiler profiler;
    return profiler;
}

} // namespace AXIOM