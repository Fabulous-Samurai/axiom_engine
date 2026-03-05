/**
 * @file daemon_engine.cpp
 * @brief AXIOM Engine v3.1 - Hardened Daemon Implementation
 *
 * Core updates:
 * - IPC: SPSC Lock-free ring buffer integration.
 * - Concurrency: OS-bypass spin-wait mechanisms via std::this_thread::yield().
 * - Memory: Rvalue references deployed for zero-copy request dispatch.
 */

#include "../include/daemon_engine.h"
#include "../include/dynamic_calc.h"

#include <iostream>
#include <sstream>
#include <stdexcept>
#include <cstring>
#include <cerrno>
#include <cstdlib>
#include <type_traits>
#include <variant>

namespace AXIOM {

namespace {

uint32_t read_env_u32(const char* name, uint32_t fallback)
{
    const char* raw = std::getenv(name);
    if (raw == nullptr || *raw == '\0')
    {
        return fallback;
    }

    char* end = nullptr;
    const unsigned long parsed = std::strtoul(raw, &end, 10);
    if (end == raw || *end != '\0' || parsed == 0UL)
    {
        return fallback;
    }

    return static_cast<uint32_t>(parsed);
}

int64_t read_env_i64(const char* name, int64_t fallback)
{
    const char* raw = std::getenv(name);
    if (raw == nullptr || *raw == '\0')
    {
        return fallback;
    }

    char* end = nullptr;
    const long long parsed = std::strtoll(raw, &end, 10);
    if (end == raw || *end != '\0' || parsed <= 0LL)
    {
        return fallback;
    }

    return static_cast<int64_t>(parsed);
}

uint32_t circuit_failure_threshold()
{
    static const uint32_t value =
        read_env_u32("AXIOM_DAEMON_CIRCUIT_FAILURE_THRESHOLD", 5U);
    return value;
}

int64_t circuit_open_duration_ms()
{
    static const int64_t value =
        read_env_i64("AXIOM_DAEMON_CIRCUIT_OPEN_MS", 2000LL);
    return value;
}

int64_t backpressure_wait_ms()
{
    static const int64_t value =
        read_env_i64("AXIOM_DAEMON_BACKPRESSURE_WAIT_MS", 5LL);
    return value;
}

int64_t now_ms()
{
    return std::chrono::duration_cast<std::chrono::milliseconds>(
               std::chrono::steady_clock::now().time_since_epoch())
        .count();
}

const char* calc_error_to_string(CalcErr err)
{
    switch (err)
    {
        case CalcErr::None: return "None";
        case CalcErr::DivideByZero: return "DivideByZero";
        case CalcErr::IndeterminateResult: return "IndeterminateResult";
        case CalcErr::OperationNotFound: return "OperationNotFound";
        case CalcErr::ArgumentMismatch: return "ArgumentMismatch";
        case CalcErr::NegativeRoot: return "NegativeRoot";
        case CalcErr::DomainError: return "DomainError";
        case CalcErr::ParseError: return "ParseError";
        case CalcErr::NumericOverflow: return "NumericOverflow";
        case CalcErr::StackOverflow: return "StackOverflow";
        case CalcErr::MemoryExhausted: return "MemoryExhausted";
        case CalcErr::InfiniteLoop: return "InfiniteLoop";
        default: return "UnknownCalcErr";
    }
}

const char* linalg_error_to_string(LinAlgErr err)
{
    switch (err)
    {
        case LinAlgErr::None: return "None";
        case LinAlgErr::NoSolution: return "NoSolution";
        case LinAlgErr::InfiniteSolutions: return "InfiniteSolutions";
        case LinAlgErr::MatrixMismatch: return "MatrixMismatch";
        case LinAlgErr::ParseError: return "ParseError";
        default: return "UnknownLinAlgErr";
    }
}

std::string engine_error_to_string(const EngineErrorResult& err)
{
    return std::visit([](const auto& e) -> std::string {
        using T = std::decay_t<decltype(e)>;
        if constexpr (std::is_same_v<T, CalcErr>)
            return calc_error_to_string(e);
        if constexpr (std::is_same_v<T, LinAlgErr>)
            return linalg_error_to_string(e);
        return "UnknownEngineError";
    }, err);
}

} // namespace

// ---------------------------------------------------------------------------
// Constructor / Destructor
// ---------------------------------------------------------------------------

DaemonEngine::DaemonEngine(const std::string& pipe_name)
    : pipe_name_(pipe_name)
    , startup_time_(std::chrono::steady_clock::now())
{
#ifdef _WIN32
    pipe_handle_ = INVALID_HANDLE_VALUE;
#else
    pipe_fd_ = -1;
#endif
}

DaemonEngine::~DaemonEngine() noexcept
{
    if (!running_.load(std::memory_order_acquire))
    {
        return;
    }

    try
    {
        stop();
    }
    catch (...)
    {
        // Destructors must not throw; swallow shutdown failures.
    }
}

// ---------------------------------------------------------------------------
// Lifecycle
// ---------------------------------------------------------------------------

bool DaemonEngine::start()
{
    bool expected = false;
    if (!running_.compare_exchange_strong(expected, true,
                                          std::memory_order_release,
                                          std::memory_order_relaxed))
    {
        return false; // already running
    }

    status_.store(DaemonStatus::STARTING, std::memory_order_release);

    PipeError err = setup_pipe();
    if (err != PipeError::None)
    {
        std::cerr << "[AXIOM Daemon] setup_pipe failed: "
                  << pipe_error_to_string(err) << '\n';
        running_.store(false, std::memory_order_release);
        status_.store(DaemonStatus::PIPE_ERROR, std::memory_order_release);
        return false;
    }

    status_.store(DaemonStatus::READY, std::memory_order_release);
    daemon_thread_    = std::thread([this] { daemon_loop(); });
    request_processor_ = std::thread([this] { request_processor_loop(); });
    return true;
}

void DaemonEngine::stop() noexcept
{
    bool expected = true;
    if (!running_.compare_exchange_strong(expected, false,
                                          std::memory_order_release,
                                          std::memory_order_relaxed))
    {
        return;
    }

    status_.store(DaemonStatus::SHUTDOWN, std::memory_order_release);

    try
    {
        if (daemon_thread_.joinable())
        {
            daemon_thread_.join();
        }
        if (request_processor_.joinable())
        {
            request_processor_.join();
        }

        cleanup_pipe();

        std::scoped_lock lock(sessions_mutex_);
        sessions_.clear();
    }
    catch (...)
    {
        // Keep noexcept contract: errors during teardown are intentionally ignored.
    }
}

// ---------------------------------------------------------------------------
// Pipe setup / teardown
// ---------------------------------------------------------------------------

DaemonEngine::PipeError DaemonEngine::setup_pipe()
{
#ifdef _WIN32
    std::string full_name = "\\\\.\\pipe\\" + pipe_name_;
    pipe_handle_ = CreateNamedPipeA(
        full_name.c_str(),
        PIPE_ACCESS_DUPLEX,
        PIPE_TYPE_MESSAGE | PIPE_READMODE_MESSAGE | PIPE_WAIT,
        1,       // max instances
        4096,    // out buffer
        4096,    // in buffer
        0,       // default timeout
        nullptr  // default security
    );
    if (pipe_handle_ == INVALID_HANDLE_VALUE)
    {
        DWORD err = GetLastError();
        if (err == ERROR_ACCESS_DENIED)    return PipeError::PermissionDenied;
        if (err == ERROR_ALREADY_EXISTS)   return PipeError::AlreadyExists;
        return PipeError::SystemError;
    }
    return PipeError::None;
#else
    std::string path = "/tmp/" + pipe_name_;
    // Remove stale FIFO
    ::unlink(path.c_str());
    if (::mkfifo(path.c_str(), 0600) != 0)
    {
        if (errno == EACCES) return PipeError::PermissionDenied;
        return PipeError::SystemError;
    }
    pipe_fd_ = ::open(path.c_str(), O_RDONLY | O_NONBLOCK);
    if (pipe_fd_ < 0) return PipeError::SystemError;
    return PipeError::None;
#endif
}

void DaemonEngine::cleanup_pipe()
{
#ifdef _WIN32
    if (pipe_handle_ != INVALID_HANDLE_VALUE)
    {
        CloseHandle(pipe_handle_);
        pipe_handle_ = INVALID_HANDLE_VALUE;
    }
#else
    if (pipe_fd_ >= 0)
    {
        ::close(pipe_fd_);
        pipe_fd_ = -1;
    }
    std::string path = "/tmp/" + pipe_name_;
    ::unlink(path.c_str());
#endif
}

const char* DaemonEngine::pipe_error_to_string(PipeError error)
{
    switch (error) {
        case PipeError::None:                    return "None";
        case PipeError::PermissionDenied:        return "PermissionDenied";
        case PipeError::AlreadyExists:           return "AlreadyExists";
        case PipeError::ResourceExhausted:       return "ResourceExhausted";
        case PipeError::InvalidName:             return "InvalidName";
        case PipeError::SystemError:             return "SystemError";
        case PipeError::SecurityDescriptorFailed:return "SecurityDescriptorFailed";
        default:                                 return "Unknown";
    }
}

// ---------------------------------------------------------------------------
// Internal helpers
// ---------------------------------------------------------------------------

// Lightweight JSON key extractor — finds "key":"value" patterns.
// Expected format: {"command":"2+2","session":"abc","mode":"algebraic"}
static std::string json_get(const std::string& json, const std::string& key)
{
    std::string search = "\"" + key + "\":\"";
    auto pos = json.find(search);
    if (pos == std::string::npos) return "";
    pos += search.size();
    auto end = json.find('"', pos);
    if (end == std::string::npos) return "";
    return json.substr(pos, end - pos);
}

// ---------------------------------------------------------------------------
// IPC loops
// ---------------------------------------------------------------------------

void DaemonEngine::daemon_loop()
{
    while (running_.load(std::memory_order_acquire))
    {
        try
        {
#ifdef _WIN32
            bool connected = ConnectNamedPipe(pipe_handle_, nullptr)
                             ? true
                             : (GetLastError() == ERROR_PIPE_CONNECTED);
            if (connected)
            {
                char buffer[4096];
                DWORD bytes_read = 0;

                if (ReadFile(pipe_handle_, buffer, sizeof(buffer) - 1, &bytes_read, nullptr))
                {
                    buffer[bytes_read] = '\0';
                    std::string req_str(buffer);

                    Request request;
                    request.command    = json_get(req_str, "command");
                    request.session_id = json_get(req_str, "session");
                    request.mode       = json_get(req_str, "mode");

                    if (!request.command.empty())
                    {
                        request.request_id = next_request_id_.fetch_add(1, std::memory_order_relaxed);
                        request.timestamp  = std::chrono::steady_clock::now();

                        // Process synchronously so we can write the response
                        // before the pipe connection is closed.
                        auto resp = execute_command(request);

                        std::string resp_json =
                            "{\"success\":" + std::string(resp.success ? "true" : "false") +
                            ",\"result\":\"" + resp.result + "\"" +
                            ",\"error\":\"" + resp.error + "\"" +
                            ",\"time\":" + std::to_string(resp.execution_time_ms) + "}";

                        DWORD written = 0;
                        WriteFile(pipe_handle_,
                                  resp_json.c_str(),
                                  static_cast<DWORD>(resp_json.size()),
                                  &written,
                                  nullptr);
                    }
                }
                DisconnectNamedPipe(pipe_handle_);
            }
#else
            char buffer[4096];
            ssize_t bytes_read = ::read(pipe_fd_, buffer, sizeof(buffer) - 1);

            if (bytes_read > 0)
            {
                buffer[bytes_read] = '\0';
                std::string req_str(buffer);

                Request request;
                request.command    = json_get(req_str, "command");
                request.session_id = json_get(req_str, "session");
                request.mode       = json_get(req_str, "mode");

                if (!request.command.empty())
                {
                    request.request_id = next_request_id_.fetch_add(1, std::memory_order_relaxed);
                    request.timestamp  = std::chrono::steady_clock::now();

                    static constexpr auto kLoopBudget = std::chrono::milliseconds(2);
                    const auto deadline = std::chrono::steady_clock::now() + kLoopBudget;
                    bool enqueued = false;
                    while (running_.load(std::memory_order_acquire) && std::chrono::steady_clock::now() < deadline)
                    {
                        if (request_queue_.push(request))
                        {
                            enqueued = true;
                            break;
                        }
                        std::this_thread::yield();
                    }

                    if (!enqueued)
                    {
                        rejected_requests_.fetch_add(1, std::memory_order_relaxed);
                    }
                }
            }
            else
            {
                std::this_thread::sleep_for(std::chrono::milliseconds(1));
            }
#endif
        }
        catch (const std::exception& e)
        {
            std::cerr << "[AXIOM Daemon] loop exception: " << e.what() << '\n';
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
    }
}

void DaemonEngine::request_processor_loop()
{
    while (running_.load(std::memory_order_acquire))
    {
        Request request;

        while (!request_queue_.pop(request)) {
            if (!running_.load(std::memory_order_acquire)) return;
            std::this_thread::yield();
        }

        status_.store(DaemonStatus::BUSY, std::memory_order_release);

        auto resp = execute_command(request);
        (void)resp; // async path: responses for send_command() callers are not piped back

        status_.store(DaemonStatus::READY, std::memory_order_release);
        total_requests_.fetch_add(1, std::memory_order_relaxed);
    }
}

// ---------------------------------------------------------------------------
// Command execution
// ---------------------------------------------------------------------------

DaemonEngine::Response DaemonEngine::execute_command(const Request& req)
{
    Response resp;
    resp.request_id        = req.request_id;
    resp.session_id        = req.session_id;
    resp.timestamp         = std::chrono::steady_clock::now();

    auto t0 = std::chrono::high_resolution_clock::now();
    try
    {
        const auto open_until = circuit_open_until_ms_.load(std::memory_order_acquire);
        const auto now = now_ms();
        if (open_until > now)
        {
            resp.success = false;
            resp.error = "CircuitBreakerOpen";
            rejected_requests_.fetch_add(1, std::memory_order_relaxed);
            return resp;
        }

        thread_local DynamicCalc calc;

        // Set calculation mode from request
        if (req.mode == "linear" || req.mode == "linear_system")
            calc.SetMode(CalculationMode::LINEAR_SYSTEM);
        else if (req.mode == "stats" || req.mode == "statistics")
            calc.SetMode(CalculationMode::STATISTICS);
        else if (req.mode == "symbolic")
            calc.SetMode(CalculationMode::SYMBOLIC);
        else
            calc.SetMode(CalculationMode::ALGEBRAIC);

        auto engine_result = calc.Evaluate(req.command);

        if (engine_result.HasErrors())
        {
            resp.success = false;
            if (engine_result.error.has_value())
            {
                resp.error = engine_error_to_string(engine_result.error.value());
            }
            else
            {
                resp.error = "EngineError";
            }
        }
        else
        {
            resp.success = true;
            auto d = engine_result.GetDouble();
            resp.result  = d ? std::to_string(*d) : "ok";
        }
    }
    catch (const std::exception& e)
    {
        resp.success = false;
        resp.error   = e.what();
    }

    auto t1 = std::chrono::high_resolution_clock::now();
    resp.execution_time_ms =
        std::chrono::duration<double, std::milli>(t1 - t0).count();

    if (resp.success)
    {
        consecutive_failures_.store(0, std::memory_order_release);
    }
    else
    {
        const auto failures = consecutive_failures_.fetch_add(1, std::memory_order_acq_rel) + 1;
        if (failures >= circuit_failure_threshold())
        {
            circuit_open_until_ms_.store(now_ms() + circuit_open_duration_ms(), std::memory_order_release);
            consecutive_failures_.store(0, std::memory_order_release);
        }
    }

    update_metrics(resp.execution_time_ms);
    return resp;
}

void DaemonEngine::update_metrics(double execution_time)
{
    uint64_t n = total_requests_.load(std::memory_order_relaxed) + 1;
    double current_avg = avg_response_time_.load(std::memory_order_relaxed);
    double new_avg = current_avg + (execution_time - current_avg) / static_cast<double>(n);
    avg_response_time_.store(new_avg, std::memory_order_relaxed);
}

// ---------------------------------------------------------------------------
// Public API
// ---------------------------------------------------------------------------

DaemonEngine::Response DaemonEngine::process_request(const Request& request)
{
    return execute_command(request);
}

bool DaemonEngine::send_command(const std::string& session_id,
                                const std::string& command,
                                const std::string& mode)
{
    if (!running_.load(std::memory_order_acquire)) return false;

    const auto open_until = circuit_open_until_ms_.load(std::memory_order_acquire);
    if (open_until > now_ms())
    {
        rejected_requests_.fetch_add(1, std::memory_order_relaxed);
        return false;
    }

    Request req;
    req.session_id = session_id;
    req.command    = command;
    req.mode       = mode;
    req.request_id = next_request_id_.fetch_add(1, std::memory_order_relaxed);
    req.timestamp  = std::chrono::steady_clock::now();

    // Apply bounded backpressure: fail fast if queue remains saturated.
    const auto deadline = std::chrono::steady_clock::now() + std::chrono::milliseconds(backpressure_wait_ms());
    while (!request_queue_.push(req)) {
        if (!running_.load(std::memory_order_acquire))
        {
            return false;
        }
        if (std::chrono::steady_clock::now() >= deadline)
        {
            rejected_requests_.fetch_add(1, std::memory_order_relaxed);
            return false;
        }
        std::this_thread::yield();
    }
    return true;
}

std::string DaemonEngine::create_session()
{
    // Use a nanosecond timestamp for a unique-enough ID without external deps
    auto ns = std::chrono::steady_clock::now().time_since_epoch().count();
    std::string id = "session_" + std::to_string(ns);

    auto ctx = std::make_unique<SessionContext>();
    ctx->session_id  = id;
    ctx->created_at  = std::chrono::steady_clock::now();

    std::scoped_lock lock(sessions_mutex_);
    sessions_[id] = std::move(ctx);
    return id;
}

bool DaemonEngine::destroy_session(const std::string& session_id)
{
    std::scoped_lock lock(sessions_mutex_);
    return sessions_.erase(session_id) > 0;
}

std::vector<std::string> DaemonEngine::get_active_sessions()
{
    std::scoped_lock lock(sessions_mutex_);
    std::vector<std::string> ids;
    ids.reserve(sessions_.size());
    for (const auto& [k, _] : sessions_) ids.push_back(k);
    return ids;
}

std::chrono::milliseconds DaemonEngine::get_uptime() const
{
    auto now = std::chrono::steady_clock::now();
    return std::chrono::duration_cast<std::chrono::milliseconds>(now - startup_time_);
}

} // namespace AXIOM
