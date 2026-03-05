/**
 * @file daemon_engine.h
 * @brief AXIOM Engine v3.1 - Enterprise Daemon Mode Architecture
 * * High-performance persistent computation daemon with:
 * - Named pipe communication (Windows/Linux)
 * - Memory-resident state management
 * - Lock-free SPSC request queue for zero-latency IPC
 * - OS-Bypass threading architecture
 */

#pragma once

#include <string>
#include <memory>
#include <atomic>
#include <thread>
#include <mutex>
#include <unordered_map>
#include <vector>
#include <chrono>

#ifdef _WIN32
    #include <windows.h>
#else
    #include <sys/stat.h>
    #include <fcntl.h>
    #include <unistd.h>
#endif

namespace AXIOM {

/**
 * @brief High-performance Single-Producer Single-Consumer Lock-Free Queue
 * @tparam T Type of the elements
 * @tparam Capacity Maximum capacity (MUST be a power of 2 for bitwise modulo)
 */
template<typename T, size_t Capacity>
class LockFreeRingBuffer {
    static_assert((Capacity & (Capacity - 1)) == 0, "Capacity must be a power of 2");
    
    T buffer_[Capacity];
    
    // Hardware-level alignment to prevent False Sharing between CPU cores
    alignas(64) std::atomic<size_t> head_{0}; 
    alignas(64) std::atomic<size_t> tail_{0}; 

public:
    // Move-semantics enabled for zero-copy string transfers
    bool push(T item) {
        size_t current_tail = tail_.load(std::memory_order_acquire);
        size_t current_head = head_.load(std::memory_order_relaxed);
        
        if (current_head - current_tail >= Capacity) return false; 
        
        buffer_[current_head & (Capacity - 1)] = std::move(item);
        head_.store(current_head + 1, std::memory_order_release);
        
        return true;
    }

    bool pop(T& item) {
        size_t current_head = head_.load(std::memory_order_acquire);
        size_t current_tail = tail_.load(std::memory_order_relaxed);
        
        if (current_head == current_tail) return false; 
        
        item = std::move(buffer_[current_tail & (Capacity - 1)]);
        tail_.store(current_tail + 1, std::memory_order_release);
        
        return true;
    }
};

class DaemonEngine {
public:
    struct Request {
        std::string session_id;
        std::string command;
        std::string mode;
        std::chrono::steady_clock::time_point timestamp;
        uint64_t request_id;
    };

    struct Response {
        uint64_t request_id;
        bool success;
        std::string result;
        std::string error;
        double execution_time_ms;
        std::string session_id;
        std::chrono::steady_clock::time_point timestamp;
    };

    enum class DaemonStatus { STARTING, READY, BUSY, PIPE_ERROR, SHUTDOWN };
    enum class PipeError { None, PermissionDenied, AlreadyExists, ResourceExhausted, InvalidName, SystemError, SecurityDescriptorFailed, Unknown };

private:
    std::atomic<DaemonStatus> status_{DaemonStatus::STARTING};
    std::atomic<bool> running_{false};
    std::atomic<uint64_t> next_request_id_{1};
    
    std::string pipe_name_;
    std::thread daemon_thread_;
    std::thread request_processor_;
    
    // Zero-latency request queue replacing std::queue and std::mutex
    LockFreeRingBuffer<Request, 1024> request_queue_;
    
    std::unordered_map<std::string, std::unique_ptr<class SessionContext>> sessions_;
    std::mutex sessions_mutex_; // Maintained solely for slow-path session initialization
    
    std::atomic<uint64_t> total_requests_{0};
    std::atomic<uint64_t> rejected_requests_{0};
    std::atomic<double> avg_response_time_{0.0};
    std::atomic<uint32_t> consecutive_failures_{0};
    std::atomic<int64_t> circuit_open_until_ms_{0};
    std::chrono::steady_clock::time_point startup_time_;

#ifdef _WIN32
    HANDLE pipe_handle_;
#else
    int pipe_fd_;
#endif

public:
    explicit DaemonEngine(const std::string& pipe_name = "axiom_daemon");
    ~DaemonEngine() noexcept;

    // Prevent copying and moving of the core daemon
    DaemonEngine(const DaemonEngine&) = delete;
    DaemonEngine& operator=(const DaemonEngine&) = delete;

    bool start();
    void stop() noexcept;
    bool is_running() const { return running_.load(std::memory_order_acquire); }
    DaemonStatus get_status() const { return status_.load(std::memory_order_acquire); }

    Response process_request(const Request& request);
    bool send_command(const std::string& session_id, const std::string& command, const std::string& mode = "algebraic");

    std::string create_session();
    bool destroy_session(const std::string& session_id);
    std::vector<std::string> get_active_sessions();

    uint64_t get_total_requests() const { return total_requests_.load(std::memory_order_relaxed); }
    uint64_t get_rejected_requests() const { return rejected_requests_.load(std::memory_order_relaxed); }
    double get_avg_response_time() const { return avg_response_time_.load(std::memory_order_relaxed); }
    std::chrono::milliseconds get_uptime() const;

private:
    void daemon_loop();
    void request_processor_loop();
    PipeError setup_pipe();
    void cleanup_pipe();
    static const char* pipe_error_to_string(PipeError error);
    Response execute_command(const Request& request);
    void update_metrics(double execution_time);
};

// SessionContext: per-session state for daemon mode
struct SessionContext {
    std::string session_id;
    std::string current_mode{"algebraic"};
    std::chrono::steady_clock::time_point created_at;
    uint64_t request_count{0};
};

} // namespace AXIOM