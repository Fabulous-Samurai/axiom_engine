/**
 * @file daemon_engine.h
 * @brief AXIOM Engine v3.0 - Enterprise Daemon Mode Architecture
 * 
 * High-performance persistent computation daemon with:
 * - Named pipe communication (Windows/Linux)
 * - Memory-resident state management
 * - Enterprise-grade session handling
 * - Zero-latency kernel persistence
 */

#pragma once

#include <string>
#include <memory>
#include <atomic>
#include <thread>
#include <mutex>
#include <unordered_map>
#include <queue>
#include <condition_variable>
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
 * @brief Enterprise Daemon Communication Protocol
 * 
 * High-performance IPC mechanism for zero-latency computation
 */
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

    enum class DaemonStatus {
        STARTING,
        READY,
        BUSY,
        PIPE_ERROR,
        SHUTDOWN
    };

    enum class PipeError {
        None,
        PermissionDenied,
        AlreadyExists,
        ResourceExhausted,
        InvalidName,
        SystemError,
        SecurityDescriptorFailed,
        Unknown
    };

private:
    // Core daemon state
    std::atomic<DaemonStatus> status_{DaemonStatus::STARTING};
    std::atomic<bool> running_{false};
    std::atomic<uint64_t> next_request_id_{1};
    
    // Communication infrastructure
    std::string pipe_name_;
    std::thread daemon_thread_;
    std::thread request_processor_;
    
    // Request queue management
    std::queue<Request> request_queue_;
    std::mutex queue_mutex_;
    std::condition_variable queue_cv_;
    
    // Session management
    std::unordered_map<std::string, std::unique_ptr<class SessionContext>> sessions_;
    std::mutex sessions_mutex_;
    
    // Performance metrics
    std::atomic<uint64_t> total_requests_{0};
    std::atomic<double> avg_response_time_{0.0};
    std::chrono::steady_clock::time_point startup_time_;

#ifdef _WIN32
    HANDLE pipe_handle_;
#else
    int pipe_fd_;
#endif

public:
    DaemonEngine(const std::string& pipe_name = "axiom_daemon");
    ~DaemonEngine();

    // Lifecycle management
    bool start();
    void stop();
    bool is_running() const { return running_.load(); }
    DaemonStatus get_status() const { return status_.load(); }

    // Communication interface
    Response process_request(const Request& request);
    bool send_command(const std::string& session_id, const std::string& command, 
                     const std::string& mode = "algebraic");

    // Session management
    std::string create_session();
    bool destroy_session(const std::string& session_id);
    std::vector<std::string> get_active_sessions();

    // Performance monitoring
    uint64_t get_total_requests() const { return total_requests_.load(); }
    double get_avg_response_time() const { return avg_response_time_.load(); }
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

/**
 * @brief Persistent Session Context
 * 
 * Memory-resident computation state for enterprise workflows
 */
class SessionContext {
public:
    std::string session_id;
    std::string current_mode;
    std::unordered_map<std::string, double> variables;
    std::vector<std::string> history;
    std::chrono::steady_clock::time_point created_at;
    std::chrono::steady_clock::time_point last_access;
    
    // Python/computation state
    std::unique_ptr<class PythonEngine> python_engine;
    std::unique_ptr<class AlgebraicParser> algebraic_parser;
    std::unique_ptr<class LinearSystemParser> linear_parser;
    
    SessionContext(const std::string& id);
    ~SessionContext();
    
    void update_access_time() {
        last_access = std::chrono::steady_clock::now();
    }
    
    std::chrono::minutes get_idle_time() const {
        auto now = std::chrono::steady_clock::now();
        return std::chrono::duration_cast<std::chrono::minutes>(now - last_access);
    }
};

/**
 * @brief High-Performance Daemon Client
 * 
 * Client interface for connecting to AXIOM Daemon
 */
class DaemonClient {
private:
    std::string pipe_name_;
    std::string session_id_;
    bool connected_;
    
#ifdef _WIN32
    HANDLE pipe_handle_;
#else
    int pipe_fd_;
#endif

public:
    DaemonClient(const std::string& pipe_name = "axiom_daemon");
    ~DaemonClient();

    // Connection management
    bool connect();
    void disconnect();
    bool is_connected() const { return connected_; }

    // Command execution
    DaemonEngine::Response execute(const std::string& command, 
                                 const std::string& mode = "algebraic");
    
    // Session management
    bool create_session();
    std::string get_session_id() const { return session_id_; }
    
    // Utility methods
    static bool is_daemon_running(const std::string& pipe_name = "axiom_daemon");
    static bool start_daemon_if_needed(const std::string& pipe_name = "axiom_daemon");
};

/**
 * @brief Enterprise Process Manager
 * 
 * Manages daemon lifecycle and enterprise deployment
 */
class ProcessManager {
public:
    static bool start_daemon_process(const std::string& pipe_name = "axiom_daemon");
    static bool stop_daemon_process(const std::string& pipe_name = "axiom_daemon");
    static bool is_daemon_process_running(const std::string& pipe_name = "axiom_daemon");
    static std::vector<std::string> get_running_daemons();
    
    // Enterprise features
    static bool install_as_service();
    static bool uninstall_service();
    static bool start_service();
    static bool stop_service();
    
private:
    static constexpr const char* SERVICE_NAME = "AxiomEngine";
    static constexpr const char* SERVICE_DISPLAY_NAME = "AXIOM Engine v3.0 Scientific Computing Service";
};

} // namespace AXIOM