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
#include <array>

#ifdef _WIN32
    #include <windows.h>
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
    bool push(T item) noexcept {
        const size_t current_tail = tail_.load(std::memory_order_acquire);
        const size_t current_head = head_.load(std::memory_order_relaxed);
        
        if (current_head - current_tail >= Capacity) [[unlikely]] return false; 
        
        buffer_[current_head & (Capacity - 1)] = std::move(item);
        head_.store(current_head + 1, std::memory_order_release);
        
        return true;
    }

    bool pop(T& item) noexcept {
        const size_t current_head = head_.load(std::memory_order_acquire);
        const size_t current_tail = tail_.load(std::memory_order_relaxed);
        
        if (current_head == current_tail) [[unlikely]] return false; 
        
        item = std::move(buffer_[current_tail & (Capacity - 1)]);
        tail_.store(current_tail + 1, std::memory_order_release);
        
        return true;
    }
};

class DaemonEngine {
public:
    struct Request {
        std::string session_id{};
        std::string command{};
        std::string mode{"algebraic"};
        std::chrono::steady_clock::time_point timestamp{};
        uint64_t request_id{0};
    };

    struct Response {
        uint64_t request_id{0};
        bool success{false};
        std::string result{};
        std::string error{};
        double execution_time_ms{0.0};
        std::string session_id{};
        std::chrono::steady_clock::time_point timestamp{};
    };

    enum class DaemonStatus { STARTING, READY, BUSY, PIPE_ERROR, SHUTDOWN };
    enum class PipeError { None, PermissionDenied, AlreadyExists, ResourceExhausted, InvalidName, SystemError, SecurityDescriptorFailed, Unknown };

private:
    // SoA queue improves cache locality for producer/consumer hot-path fields.
    template<size_t Capacity>
    class RequestQueueSoA {
        static_assert((Capacity & (Capacity - 1)) == 0, "Capacity must be a power of 2");

        alignas(64) std::atomic<size_t> head_{0};
        alignas(64) std::atomic<size_t> tail_{0};

        std::array<std::string, Capacity> session_ids_{};
        std::array<std::string, Capacity> commands_{};
        std::array<std::string, Capacity> modes_{};
        std::array<std::chrono::steady_clock::time_point, Capacity> timestamps_{};
        std::array<uint64_t, Capacity> request_ids_{};

    public:
        [[nodiscard]] bool push(const Request& item) noexcept {
            const size_t current_tail = tail_.load(std::memory_order_acquire);
            const size_t current_head = head_.load(std::memory_order_relaxed);

            if (current_head - current_tail >= Capacity) [[unlikely]] {
                return false;
            }

            const size_t idx = current_head & (Capacity - 1);
            try {
                session_ids_[idx] = item.session_id;
                commands_[idx] = item.command;
                modes_[idx] = item.mode;
            } catch (...) {
                return false;
            }
            timestamps_[idx] = item.timestamp;
            request_ids_[idx] = item.request_id;
            head_.store(current_head + 1, std::memory_order_release);
            return true;
        }

        [[nodiscard]] bool pop(Request& item) noexcept {
            const size_t current_head = head_.load(std::memory_order_acquire);
            const size_t current_tail = tail_.load(std::memory_order_relaxed);

            if (current_head == current_tail) [[unlikely]] {
                return false;
            }

            const size_t idx = current_tail & (Capacity - 1);
            item.session_id = std::move(session_ids_[idx]);
            item.command = std::move(commands_[idx]);
            item.mode = std::move(modes_[idx]);
            item.timestamp = timestamps_[idx];
            item.request_id = request_ids_[idx];
            tail_.store(current_tail + 1, std::memory_order_release);
            return true;
        }
    };

    std::atomic<DaemonStatus> status_{DaemonStatus::STARTING};
    std::atomic<bool> running_{false};
    std::atomic<uint64_t> next_request_id_{1};
    
    std::string pipe_name_;
    std::jthread daemon_thread_;
    std::jthread request_processor_;
    
    // Zero-latency request queue replacing std::queue and std::mutex
    RequestQueueSoA<1024> request_queue_;
    
    std::unordered_map<std::string, std::unique_ptr<class SessionContext>> sessions_;
    std::mutex sessions_mutex_; // Maintained solely for slow-path session initialization
    
    std::atomic<uint64_t> total_requests_{0};
    std::atomic<uint64_t> rejected_requests_{0};
    std::atomic<double> avg_response_time_{0.0};
    std::atomic<uint32_t> consecutive_failures_{0};
    std::atomic<int64_t> circuit_open_until_ms_{0};
    std::atomic<uint64_t> enqueued_requests_{0};
    std::atomic<uint64_t> completed_requests_{0};
    std::chrono::steady_clock::time_point startup_time_{std::chrono::steady_clock::now()};

#ifdef _WIN32
    HANDLE pipe_handle_{INVALID_HANDLE_VALUE};
#else
    int pipe_fd_{-1};
#endif

public:
    explicit DaemonEngine(const std::string& pipe_name = "axiom_daemon");
    ~DaemonEngine() noexcept;

    // Prevent copying and moving of the core daemon
    DaemonEngine(const DaemonEngine&) = delete;
    DaemonEngine& operator=(const DaemonEngine&) = delete;

    [[nodiscard]] bool start();
    void stop() noexcept;
    [[nodiscard]] bool is_running() const noexcept { return running_.load(std::memory_order_acquire); }
    [[nodiscard]] DaemonStatus get_status() const noexcept { return status_.load(std::memory_order_acquire); }

    [[nodiscard]] Response process_request(const Request& request);
    [[nodiscard]] bool send_command(const std::string& session_id, const std::string& command, const std::string& mode = "algebraic");

    [[nodiscard]] std::string create_session();
    [[nodiscard]] bool destroy_session(const std::string& session_id);
    [[nodiscard]] std::vector<std::string> get_active_sessions();

    [[nodiscard]] uint64_t get_total_requests() const noexcept { return total_requests_.load(std::memory_order_relaxed); }
    [[nodiscard]] uint64_t get_rejected_requests() const noexcept { return rejected_requests_.load(std::memory_order_relaxed); }
    [[nodiscard]] double get_avg_response_time() const noexcept { return avg_response_time_.load(std::memory_order_relaxed); }
    [[nodiscard]] std::chrono::milliseconds get_uptime() const noexcept;

private:
    void daemon_loop();
    void process_windows_pipe();
    void process_posix_pipe();
    void request_processor_loop();
    [[nodiscard]] PipeError setup_pipe();
    void cleanup_pipe();
    static const char* pipe_error_to_string(PipeError error) noexcept;
    [[nodiscard]] Response execute_command(const Request& request);
    void update_metrics(double execution_time) noexcept;
};

// SessionContext: per-session state for daemon mode
struct SessionContext {
    std::string session_id{};
    std::string current_mode{"algebraic"};
    std::chrono::steady_clock::time_point created_at{};
    uint64_t request_count{0};
};

} // namespace AXIOM