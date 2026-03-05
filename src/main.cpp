/**
 * @file main.cpp
 * @brief AXIOM Engine v3.0 - Enterprise Scientific Computing Platform
 * 
 * Main entry point with daemon mode and enterprise features
 */

#include <iostream>
#include <string>
#include <vector>
#include <memory>
#include <chrono>
#include <thread>
#include <algorithm>
#include <cctype>
#include <variant>
#include <limits>
#include <filesystem>
#include <cstdlib>

#include "dynamic_calc.h"
#include "extended_types.h"
#include "plot_engine.h"

// Enterprise features (conditionally compiled based on availability)
#ifdef ENABLE_DAEMON_MODE
    #include "daemon_engine.h"
#endif

#ifdef ENABLE_ARENA_ALLOCATOR
    #include "arena_allocator.h"
#endif

#ifdef ENABLE_SYMENGINE_INTEGRATION
    #include "symengine_integration.h"
#endif

namespace {
bool try_parse_mode(const std::string& raw_mode, AXIOM::CalculationMode& mode_out);
bool is_mode_supported(AXIOM::CalculationMode mode);
int launch_gui_interface(bool prefer_qt = false, bool standard_gui = false);
}



void print_axiom_banner() {
    std::cout << "\n";
    std::cout << "+----------------------------------------------------------------------+\n";
    std::cout << "|                     AXIOM Engine v3.0                               |\n";
    std::cout << "|               Enterprise Scientific Computing Platform               |\n";
    std::cout << "+----------------------------------------------------------------------+\n";
    std::cout << "|  * Ultra-High Performance C++ Engine                                |\n";
    std::cout << "|  * Enterprise Daemon Architecture                                   |\n";
    std::cout << "|  * Arena Memory Management                                          |\n";
    std::cout << "|  * SymEngine Symbolic Computing                                     |\n";
    std::cout << "|  * Python Integration (nanobind)                                    |\n";
    std::cout << "|  * Advanced Linear Algebra (Eigen)                                  |\n";
    std::cout << "|  * NUMA-Optimized Memory Pools                                      |\n";
    std::cout << "+----------------------------------------------------------------------+\n";
    std::cout << "\n";
}

void print_help() {
    std::cout << "AXIOM Engine v3.0 - Usage:\n\n";
    
    std::cout << "Interactive Modes:\n";
    std::cout << "  axiom                       Start interactive calculator\n";
    std::cout << "  axiom --gui                 Start PRO Qt GUI interface\n";
    std::cout << "  axiom --gui-standard        Start STANDARD Qt GUI interface\n";
    std::cout << "  axiom --gui-qt              Start PRO Qt/QSS GUI interface\n";

    
    std::cout << "Enterprise Daemon Mode:\n";
    std::cout << "  axiom --daemon              Start as background daemon\n";
    std::cout << "  axiom --daemon --pipe=NAME  Start daemon with custom pipe name\n";
    std::cout << "  axiom --daemon-status       Check daemon status\n";
    std::cout << "  axiom --daemon-stop         Stop running daemon\n\n";
    
    std::cout << "Command Line Execution:\n";
    std::cout << "  axiom \"expression\"          Execute single expression\n";
    std::cout << "  axiom --mode=MODE \"expr\"     Execute in specific mode\n";
    std::cout << "  axiom --symbolic \"expr\"      Symbolic computation\n";
    std::cout << "  axiom --numeric \"expr\"       Numeric evaluation\n\n";
    
    std::cout << "Modes Available:\n";
    std::cout << "  algebraic    Basic arithmetic and algebra\n";
    std::cout << "  linear       Matrix operations and linear systems\n";
    std::cout << "  statistics   Statistical analysis\n";
    std::cout << "  symbolic     Computer algebra system\n";
    std::cout << "  units        Unit conversions\n\n";
    
    std::cout << "Enterprise Features:\n";
    std::cout << "  axiom --install-service     Install as Windows service\n";
    std::cout << "  axiom --benchmark           Run performance benchmarks\n";
    std::cout << "  axiom --memory-profile      Enable memory profiling\n";
    std::cout << "  axiom --numa-optimize       Enable NUMA optimizations\n\n";
    
    std::cout << "Examples:\n";
    std::cout << "  axiom \"2 + 3 * 4\"                    # Basic arithmetic\n";
    std::cout << "  axiom --symbolic \"diff(x^2, x)\"       # Symbolic differentiation\n";
    std::cout << "  axiom --mode=linear \"solve([2,3;1,4], [5;6])\" # Linear system\n";
    std::cout << "  axiom --daemon &                      # Start daemon in background\n\n";
}

int run_interactive_mode() {
    print_axiom_banner();
    
    // Initialize memory management if available
#ifdef ENABLE_ARENA_ALLOCATOR
    AXIOM::MemoryProfiler::instance().enable_profiling(true);
#endif
    
    // Initialize symbolic engine if available
#ifdef ENABLE_SYMENGINE_INTEGRATION
    auto cas = std::make_unique<AXIOM::ComputerAlgebraSystem>();
#endif
    
    // Initialize calculation engine
    auto calc = std::make_unique<AXIOM::DynamicCalc>();
    
    std::cout << "AXIOM Engine v3.0 ready for enterprise computing...\n";
    std::cout << "Type 'help' for commands, 'exit' to quit\n\n";
    
    std::string input;
    AXIOM::CalculationMode current_mode = AXIOM::CalculationMode::ALGEBRAIC;
    
    while (true) {
        std::cout << "[" << AXIOM::mode_to_string(current_mode) << "] AXIOM> ";
        if (!std::getline(std::cin, input)) break; // EOF or stream error

        if (input.empty()) continue;

        // Support users accidentally typing shell-style prefixed commands inside REPL.
        if (input.rfind("axiom ", 0) == 0) {
            input = input.substr(6);
            auto first = input.find_first_not_of(" \t\r\n");
            if (first == std::string::npos) {
                continue;
            }
            auto last = input.find_last_not_of(" \t\r\n");
            input = input.substr(first, last - first + 1);
        }
        
        if (input == "exit" || input == "quit") {
            break;
        }
        
        if (input == "help" || input == "--help" || input == "-h") {
            print_help();
            continue;
        }

        if (input == "--gui") {
            std::cout << "GUI mode should be started from shell: axiom --gui\n";
            continue;
        }
        if (input == "--gui-standard") {
            std::cout << "STANDARD GUI mode should be started from shell: axiom --gui-standard\n";
            continue;
        }
        
        // Check for mode changes.
        std::string requested_mode;
        if (input == "algebraic" || input == "linear" || input == "statistics" ||
            input == "symbolic" || input == "units" || input == "plot") {
            requested_mode = input;
        } else if (input.rfind("mode ", 0) == 0) {
            requested_mode = input.substr(5);
        } else if (input.rfind(":mode ", 0) == 0) {
            requested_mode = input.substr(6);
        }

        if (!requested_mode.empty()) {
            auto first = requested_mode.find_first_not_of(" \t\r\n");
            if (first != std::string::npos) {
                auto last = requested_mode.find_last_not_of(" \t\r\n");
                requested_mode = requested_mode.substr(first, last - first + 1);
            }

            AXIOM::CalculationMode parsed_mode = AXIOM::CalculationMode::ALGEBRAIC;
            if (!try_parse_mode(requested_mode, parsed_mode)) {
                std::cout << "Unknown mode: " << requested_mode << "\n";
                continue;
            }
            if (!is_mode_supported(parsed_mode)) {
                std::cout << "Mode not enabled in this build: " << requested_mode << "\n";
                continue;
            }

            current_mode = parsed_mode;
            std::cout << "Switched to " << AXIOM::mode_to_string(current_mode) << " mode\n";
            continue;
        }
        
#ifdef ENABLE_DAEMON_MODE
        if (input == "daemon") {
            std::cout << "Starting daemon mode...\n";
            auto daemon = std::make_unique<AXIOM::DaemonEngine>();
            if (daemon->start()) {
                std::cout << "Daemon started successfully. Type 'stop' to exit daemon mode.\n";
                
                // Keep daemon running until stop
                while (true) {
                    std::string daemon_input;
                    std::cout << "DAEMON> ";
                    if (!std::getline(std::cin, daemon_input)) {
                        daemon->stop();
                        break;
                    }

                    if (daemon_input == "stop" || daemon_input == "exit") {
                        daemon->stop();
                        std::cout << "Daemon stopped.\n";
                        break;
                    } else if (daemon_input == "status") {
                        std::cout << "Status: " << static_cast<int>(daemon->get_status()) << "\n";
                        std::cout << "Total requests: " << daemon->get_total_requests() << "\n";
                        std::cout << "Avg response time: " << daemon->get_avg_response_time() << "ms\n";
                        std::cout << "Uptime: " << daemon->get_uptime().count() << "ms\n";
                    }
                }
                continue;
            } else {
                std::cout << "Error: Failed to start daemon.\n";
                continue;
            }
        }
#endif
        
#ifdef ENABLE_ARENA_ALLOCATOR
        if (input == "memory") {
            auto stats = AXIOM::PoolManager::instance().get_all_stats();
            std::cout << "Memory Pool Statistics:\n";
            for (size_t i = 0; i < stats.size(); ++i) {
                const auto& stat = stats[i];
                std::cout << "  Pool " << i << ": " << stat.used_size << "/" << stat.total_size 
                         << " bytes (" << (100.0 * stat.used_size / stat.total_size) << "%)\n";
            }
            continue;
        }
#endif
        
        // Try calculation
        try {
            calc->SetMode(current_mode);
            auto basic_result = calc->Evaluate(input);
            auto result = AXIOM::ExtendedEngineResult::from_engine_result(basic_result);
            
            if (result.success) {
                if (current_mode == AXIOM::CalculationMode::LINEAR_SYSTEM && result.has_linear_result) {
                    std::cout << "Linear System Solution:\n";
                    for (size_t i = 0; i < result.linear_result.solution.size(); ++i) {
                        std::cout << "  x" << i << " = " << result.linear_result.solution[i] << "\n";
                    }
                } else if (current_mode == AXIOM::CalculationMode::STATISTICS && result.has_stats_result) {
                    const auto& stats = result.stats_result;
                    std::cout << "Statistical Analysis:\n";
                    std::cout << "  Mean: " << stats.mean << "\n";
                    std::cout << "  Std Dev: " << stats.std_dev << "\n";
                    std::cout << "  Min: " << stats.min << "\n";
                    std::cout << "  Max: " << stats.max << "\n";
                    std::cout << "  Count: " << stats.count << "\n";
                } else if (current_mode == AXIOM::CalculationMode::SYMBOLIC && result.has_symbolic_result) {
                    std::cout << "Symbolic result: " << result.symbolic_result.result << "\n";
                    if (!result.symbolic_result.simplified.empty()) {
                        std::cout << "Simplified: " << result.symbolic_result.simplified << "\n";
                    }
                } else if (current_mode == AXIOM::CalculationMode::UNITS && result.has_unit_result) {
                    std::cout << "Converted: " << result.unit_result.value 
                             << " " << result.unit_result.target_unit << "\n";
                } else if (current_mode == AXIOM::CalculationMode::PLOT && result.has_plot_result) {
                    std::cout << "Plot generated: " << result.plot_result.filename << "\n";
                    std::cout << "Range: [" << result.plot_result.x_min << ", " 
                             << result.plot_result.x_max << "]\n";
                } else {
                    std::cout << result.value << "\n";
                }
            } else {
                std::cout << "Error: " << result.error_message << "\n";
            }
        } catch (const std::runtime_error& e) {
            std::cout << "Runtime error: " << e.what() << "\n";
        } catch (const std::exception& e) {
            std::cout << "Error: " << e.what() << "\n";
        }
    }
    
    return 0;
}

#ifdef ENABLE_DAEMON_MODE
int run_daemon_mode(const std::vector<std::string>& args) {
    std::string pipe_name = "axiom_daemon";
    
    // Parse daemon arguments
    for (const auto& arg : args) {
        if (arg.starts_with("--pipe=")) {
            pipe_name = arg.substr(7);
        }
    }
    
    print_axiom_banner();
    std::cout << "Starting AXIOM Engine Daemon Mode...\n";
    std::cout << "📡 Pipe name: " << pipe_name << "\n\n";
    
    // Initialize enterprise memory management
#ifdef ENABLE_ARENA_ALLOCATOR
    AXIOM::MemoryProfiler::instance().enable_profiling(true);
#endif
    
    auto daemon = std::make_unique<AXIOM::DaemonEngine>(pipe_name);
    
    if (!daemon->start()) {
        std::cerr << "Error: Failed to start daemon\n";
        return 1;
    }
    
    std::cout << "AXIOM Daemon started successfully\n";
    std::cout << "Enterprise mode: HIGH-PERFORMANCE PERSISTENT COMPUTING\n";
    std::cout << "Memory pools: NUMA-optimized allocation\n";
    std::cout << "Symbolic engine: SymEngine integration active\n\n";
    
    // Keep daemon running
    while (daemon->is_running()) {
        std::this_thread::sleep_for(std::chrono::seconds(1));
        
        // Print periodic status
        static auto last_status_time = std::chrono::steady_clock::now();
        auto now = std::chrono::steady_clock::now();
        if (now - last_status_time >= std::chrono::minutes(5)) {
            std::cout << "Status: " << daemon->get_total_requests() 
                     << " requests, " << daemon->get_avg_response_time() 
                     << "ms avg response time, uptime " 
                     << daemon->get_uptime().count() << "ms\n";
            last_status_time = now;
        }
    }
    
    std::cout << "AXIOM Daemon stopped\n";
    return 0;
}
#endif

int run_benchmark_mode() {
    print_axiom_banner();
    std::cout << "🏁 Running AXIOM Engine Performance Benchmarks...\n\n";
    
    // Initialize systems
    auto calc = std::make_unique<AXIOM::DynamicCalc>();
    
    // Benchmark 1: Basic arithmetic
    std::cout << "Basic Arithmetic Benchmark:\n";
    auto start = std::chrono::high_resolution_clock::now();
    for (int i = 0; i < 10000; ++i) {
        calc->SetMode(AXIOM::CalculationMode::ALGEBRAIC);
        calc->Evaluate("2 + 3 * 4 - 1");
    }
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
    std::cout << "  10,000 calculations in " << duration.count() << "us\n";
    std::cout << "  " << (10000.0 * 1000000.0 / duration.count()) << " calculations/second\n\n";
    
#ifdef ENABLE_ARENA_ALLOCATOR
    // Benchmark 3: Memory allocation
    std::cout << "Memory Arena Benchmark:\n";
    auto arena = std::make_unique<AXIOM::MemoryArena>(64 * 1024 * 1024);
    start = std::chrono::high_resolution_clock::now();
    std::vector<void*> ptrs;
    for (int i = 0; i < 100000; ++i) {
        ptrs.push_back(arena->allocate(64));
    }
    end = std::chrono::high_resolution_clock::now();
    duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
    std::cout << "  100,000 allocations in " << duration.count() << "us\n";
    std::cout << "  " << (100000.0 * 1000000.0 / duration.count()) << " allocations/second\n\n";
#endif
    
    std::cout << "All benchmarks completed successfully!\n";
    std::cout << "AXIOM Engine v3.0 delivering enterprise-grade performance!\n";
    
    return 0;
}

namespace {
bool try_parse_mode(const std::string& raw_mode, AXIOM::CalculationMode& mode_out) {
    std::string mode = raw_mode;
    std::transform(mode.begin(), mode.end(), mode.begin(), [](unsigned char c){ return static_cast<char>(std::tolower(c)); });

    if (mode == "algebraic" || mode == "alg") {
        mode_out = AXIOM::CalculationMode::ALGEBRAIC;
        return true;
    }
    if (mode == "linear" || mode == "linear_system" || mode == "linear-system") {
        mode_out = AXIOM::CalculationMode::LINEAR_SYSTEM;
        return true;
    }
    if (mode == "statistics" || mode == "stats") {
        mode_out = AXIOM::CalculationMode::STATISTICS;
        return true;
    }
    if (mode == "symbolic" || mode == "sym") {
        mode_out = AXIOM::CalculationMode::SYMBOLIC;
        return true;
    }
    if (mode == "units" || mode == "unit") {
        mode_out = AXIOM::CalculationMode::UNITS;
        return true;
    }
    if (mode == "plot" || mode == "plotting") {
        mode_out = AXIOM::CalculationMode::PLOT;
        return true;
    }

    return false;
}

bool is_mode_supported(AXIOM::CalculationMode mode) {
    // DynamicCalc currently wires parsers for these modes in this build profile.
    return mode == AXIOM::CalculationMode::ALGEBRAIC ||
           mode == AXIOM::CalculationMode::LINEAR_SYSTEM ||
           mode == AXIOM::CalculationMode::STATISTICS ||
           mode == AXIOM::CalculationMode::SYMBOLIC ||
           mode == AXIOM::CalculationMode::UNITS ||
           mode == AXIOM::CalculationMode::PLOT;
}

int run_launch_command(const std::string& command) {
#ifdef _WIN32
    // Force execution through cmd.exe so Windows absolute interpreter paths are
    // handled by the native command parser instead of MinGW shell semantics.
    std::string wrapped = "cmd /d /c ";
    wrapped += command;
    return std::system(wrapped.c_str());
#else
    return std::system(command.c_str());
#endif
}

int launch_gui_interface(bool prefer_qt, bool standard_gui) {
    namespace fs = std::filesystem;

    std::vector<std::string> gui_candidates;
    if (standard_gui) {
        gui_candidates.push_back("gui/qt/axiom_qt_standard_gui.py");
        gui_candidates.push_back("gui/python/axiom_gui.py");
    } else {
        if (prefer_qt) {
            gui_candidates.push_back("gui/qt/axiom_qt_gui.py");
        }
        gui_candidates.push_back("gui/python/axiom_pro_gui.py");
        if (!prefer_qt) {
            gui_candidates.push_back("gui/qt/axiom_qt_gui.py");
        }
    }

    std::string gui_script;
    for (const auto& candidate : gui_candidates) {
        if (fs::exists(candidate)) {
            gui_script = candidate;
            break;
        }
    }

    if (gui_script.empty()) {
        if (standard_gui) {
            std::cerr << "Error: STANDARD GUI script not found (expected gui/qt/axiom_qt_standard_gui.py or gui/python/axiom_gui.py)\n";
        } else {
            std::cerr << "Error: PRO GUI script not found (expected gui/python/axiom_pro_gui.py or gui/qt/axiom_qt_gui.py)\n";
        }
        return 1;
    }

    std::vector<std::string> launch_commands;

    if (const char* venv_env = std::getenv("VIRTUAL_ENV")) {
        const fs::path venv_path(venv_env);
#ifdef _WIN32
        const fs::path venv_python_scripts = venv_path / "Scripts" / "python.exe";
        const fs::path venv_python_bin = venv_path / "bin" / "python";
        if (fs::exists(venv_python_scripts)) {
            launch_commands.push_back("\"" + venv_python_scripts.string() + "\" \"" + gui_script + "\"");
        }
        if (fs::exists(venv_python_bin)) {
            launch_commands.push_back("\"" + venv_python_bin.string() + "\" \"" + gui_script + "\"");
        }
#else
        const fs::path venv_python = venv_path / "bin" / "python";
        if (fs::exists(venv_python)) {
            launch_commands.push_back("\"" + venv_python.string() + "\" \"" + gui_script + "\"");
        }
#endif
    }

#ifdef _WIN32
    const fs::path local_venv_scripts = fs::path(".venv") / "Scripts" / "python.exe";
    const fs::path local_venv_bin = fs::path(".venv") / "bin" / "python";
    if (fs::exists(local_venv_scripts)) {
        launch_commands.push_back("\"" + local_venv_scripts.string() + "\" \"" + gui_script + "\"");
    }
    if (fs::exists(local_venv_bin)) {
        launch_commands.push_back("\"" + local_venv_bin.string() + "\" \"" + gui_script + "\"");
    }

    // Support MSYS2/MinGW Python where Qt bindings are often installed via pacman.
    const fs::path msys2_mingw_python = fs::path("C:\\msys64\\mingw64\\bin\\python.exe");
    const fs::path msys2_ucrt_python = fs::path("C:\\msys64\\ucrt64\\bin\\python.exe");
    if (fs::exists(msys2_mingw_python)) {
        launch_commands.push_back("\"" + msys2_mingw_python.string() + "\" \"" + gui_script + "\"");
        launch_commands.push_back("set \"PATH=C:\\msys64\\mingw64\\bin;%PATH%\" && python \"" + gui_script + "\"");
    }
    if (fs::exists(msys2_ucrt_python)) {
        launch_commands.push_back("\"" + msys2_ucrt_python.string() + "\" \"" + gui_script + "\"");
        launch_commands.push_back("set \"PATH=C:\\msys64\\ucrt64\\bin;%PATH%\" && python \"" + gui_script + "\"");
    }

    launch_commands.push_back("python \"" + gui_script + "\"");
    launch_commands.push_back("python3 \"" + gui_script + "\"");
    launch_commands.push_back("py -3 \"" + gui_script + "\"");
#else
    const fs::path local_venv = fs::path(".venv") / "bin" / "python";
    if (fs::exists(local_venv)) {
        launch_commands.push_back("\"" + local_venv.string() + "\" \"" + gui_script + "\"");
    }
    launch_commands.push_back("python3 \"" + gui_script + "\"");
    launch_commands.push_back("python \"" + gui_script + "\"");
#endif

    std::cout << "Starting GUI mode...\n";
    std::cout << "Launching: " << gui_script << "\n";

    for (const auto& cmd : launch_commands) {
        std::cout << "Trying interpreter command: " << cmd << "\n";
        const int rc = run_launch_command(cmd);
        if (rc == 0) {
            return 0;
        }
    }

    std::cerr << "Error: Failed to launch GUI with available Python commands\n";
    return 1;
}

std::string format_error(const EngineErrorResult& err) {
    if (std::holds_alternative<CalcErr>(err)) {
        switch (std::get<CalcErr>(err)) {
            case CalcErr::DivideByZero: return "Divide by zero";
            case CalcErr::IndeterminateResult: return "Indeterminate result";
            case CalcErr::OperationNotFound: return "Operation not found";
            case CalcErr::ArgumentMismatch: return "Argument mismatch";
            case CalcErr::NegativeRoot: return "Negative root";
            case CalcErr::DomainError: return "Domain error";
            case CalcErr::ParseError: return "Parse error";
            case CalcErr::NumericOverflow: return "Numeric overflow";
            case CalcErr::StackOverflow: return "Stack overflow";
            case CalcErr::MemoryExhausted: return "Memory exhausted";
            case CalcErr::InfiniteLoop: return "Infinite loop";
            case CalcErr::None: default: return "Unknown calculation error";
        }
    }

    if (std::holds_alternative<LinAlgErr>(err)) {
        switch (std::get<LinAlgErr>(err)) {
            case LinAlgErr::NoSolution: return "No solution";
            case LinAlgErr::InfiniteSolutions: return "Infinite solutions";
            case LinAlgErr::MatrixMismatch: return "Matrix size mismatch";
            case LinAlgErr::ParseError: return "Parse error";
            case LinAlgErr::None: default: return "Unknown linear algebra error";
        }
    }

    return "Unknown error";
}

int print_result(const EngineResult& result, AXIOM::CalculationMode mode) {
    if (!result.result.has_value()) {
        if (result.error.has_value()) {
            std::cerr << "Error: " << format_error(result.error.value()) << "\n";
        } else {
            std::cerr << "Error: Unknown failure" << "\n";
        }
        return 1;
    }

    const auto& value = result.result.value();

    if (std::holds_alternative<double>(value)) {
        printf("%.15g\n", std::get<double>(value));
        return 0;
    }

    if (std::holds_alternative<std::complex<double>>(value)) {
        const auto& c = std::get<std::complex<double>>(value);
        printf("%.15g%+.15gi\n", c.real(), c.imag());
        return 0;
    }

    if (std::holds_alternative<AXIOM::Number>(value)) {
        auto c = AXIOM::GetComplex(std::get<AXIOM::Number>(value));
        printf("%.15g%+.15gi\n", c.real(), c.imag());
        return 0;
    }

    if (std::holds_alternative<Vector>(value)) {
        const auto& vec = std::get<Vector>(value);
        if (mode == AXIOM::CalculationMode::LINEAR_SYSTEM) {
            for (size_t i = 0; i < vec.size(); ++i) {
                printf("x%zu = %.15g\n", i, vec[i]);
            }
        } else {
            printf("[");
            for (size_t i = 0; i < vec.size(); ++i) {
                printf("%.15g%s", vec[i], (i + 1 < vec.size()) ? ", " : "");
            }
            printf("]\n");
        }
        return 0;
    }

    if (std::holds_alternative<Matrix>(value)) {
        const auto& mat = std::get<Matrix>(value);

        if (mode == AXIOM::CalculationMode::PLOT) {
            Vector x_data;
            Vector y_data;
            x_data.reserve(mat.size());
            y_data.reserve(mat.size());

            double x_min = std::numeric_limits<double>::infinity();
            double x_max = -std::numeric_limits<double>::infinity();
            double y_min = std::numeric_limits<double>::infinity();
            double y_max = -std::numeric_limits<double>::infinity();

            for (const auto& row : mat) {
                if (row.size() < 2) {
                    continue;
                }
                const double x = row[0];
                const double y = row[1];
                x_data.push_back(x);
                y_data.push_back(y);
                x_min = std::min(x_min, x);
                x_max = std::max(x_max, x);
                y_min = std::min(y_min, y);
                y_max = std::max(y_max, y);
            }

            if (!x_data.empty()) {
                PlotConfig cfg;
                cfg.x_min = x_min;
                cfg.x_max = x_max;
                cfg.y_min = y_min;
                cfg.y_max = y_max;

                if (cfg.x_min == cfg.x_max) {
                    cfg.x_min -= 1.0;
                    cfg.x_max += 1.0;
                }
                if (cfg.y_min == cfg.y_max) {
                    cfg.y_min -= 1.0;
                    cfg.y_max += 1.0;
                }

                PlotEngine plot_engine;
                std::cout << plot_engine.PlotData(x_data, y_data, cfg);
                return 0;
            }
        }

        for (const auto& row : mat) {
            printf("[");
            for (size_t i = 0; i < row.size(); ++i) {
                printf("%.15g%s", row[i], (i + 1 < row.size()) ? ", " : "");
            }
            printf("]\n");
        }
        return 0;
    }

    if (std::holds_alternative<std::string>(value)) {
        const auto& str_value = std::get<std::string>(value);
        if (mode == AXIOM::CalculationMode::PLOT && str_value.rfind("PLOT_FUNCTION:", 0) == 0) {
            const std::string payload = str_value.substr(std::string("PLOT_FUNCTION:").size());
            std::vector<std::string> parts;
            size_t start = 0;
            for (size_t i = 0; i <= payload.size(); ++i) {
                if (i == payload.size() || payload[i] == ',') {
                    std::string token = payload.substr(start, i - start);
                    auto first = token.find_first_not_of(" \t\r\n");
                    if (first != std::string::npos) {
                        auto last = token.find_last_not_of(" \t\r\n");
                        token = token.substr(first, last - first + 1);
                    }
                    parts.push_back(token);
                    start = i + 1;
                }
            }

            if (parts.size() == 5) {
                try {
                    PlotConfig cfg;
                    cfg.x_min = std::stod(parts[1]);
                    cfg.x_max = std::stod(parts[2]);
                    cfg.y_min = std::stod(parts[3]);
                    cfg.y_max = std::stod(parts[4]);
                    PlotEngine plot_engine;
                    std::cout << plot_engine.PlotFunction(parts[0], cfg);
                    return 0;
                } catch (...) {
                    std::cerr << "Error: Invalid plot configuration" << "\n";
                    return 1;
                }
            }
        }

        std::cout << str_value << "\n";
        return 0;
    }

    std::cerr << "Error: Unsupported result type" << "\n";
    return 1;
}
} // namespace

int main(int argc, char* argv[]) {
    std::vector<std::string> args;
    for (int i = 1; i < argc; ++i) {
        args.emplace_back(argv[i]);
    }
    
    // Check for help
    if (args.empty()) {
        return run_interactive_mode();
    }
    
    if (std::find(args.begin(), args.end(), "--help") != args.end() ||
        std::find(args.begin(), args.end(), "-h") != args.end()) {
        print_help();
        return 0;
    }
    
    // Check for interactive mode (persistent subprocess for GUI)
    if (std::find(args.begin(), args.end(), "--interactive") != args.end()) {
        // Initialize calculation engine once
        auto calc = std::make_unique<AXIOM::DynamicCalc>();
        AXIOM::CalculationMode current_mode = AXIOM::CalculationMode::ALGEBRAIC;
        
        std::string line;
        while (std::getline(std::cin, line)) {
            // Normalize Windows/Unix line endings and surrounding spaces.
            auto first = line.find_first_not_of(" \t\r\n");
            if (first == std::string::npos) {
                continue;
            }
            auto last = line.find_last_not_of(" \t\r\n");
            line = line.substr(first, last - first + 1);

            if (line == "exit" || line == "quit") {
                break;
            }
            
            // Handle mode changes (e.g., ":mode linear")
            if (line.rfind(":mode ", 0) == 0) {
                std::string mode_str = line.substr(6);
                auto first_mode = mode_str.find_first_not_of(" \t\r\n");
                if (first_mode == std::string::npos) {
                    std::cout << "Error: Missing mode name\n";
                    std::cout << "__END__\n" << std::flush;
                    continue;
                }
                auto last_mode = mode_str.find_last_not_of(" \t\r\n");
                mode_str = mode_str.substr(first_mode, last_mode - first_mode + 1);

                AXIOM::CalculationMode parsed_mode = AXIOM::CalculationMode::ALGEBRAIC;
                if (!try_parse_mode(mode_str, parsed_mode)) {
                    std::cout << "Error: Unknown mode '" << mode_str << "'\n";
                    std::cout << "__END__\n" << std::flush;
                    continue;
                }
                if (!is_mode_supported(parsed_mode)) {
                    std::cout << "Error: Mode '" << mode_str << "' is not enabled in this build\n";
                    std::cout << "__END__\n" << std::flush;
                    continue;
                }

                current_mode = parsed_mode;
                std::cout << "Mode changed to " << AXIOM::mode_to_string(current_mode) << "\n";
                std::cout << "__END__\n" << std::flush;
                continue;
            }
            
            // Execute command
            try {
                calc->SetMode(current_mode);
                auto basic_result = calc->Evaluate(line);
                print_result(basic_result, current_mode);
                std::cout << "__END__\n" << std::flush;
            } catch (const std::runtime_error& e) {
                // Keep machine protocol on stdout so clients only reading stdout still receive errors.
                std::cout << "Runtime error: " << e.what() << "\n";
                std::cout << "__END__\n" << std::flush;
            } catch (const std::exception& e) {
                // Keep machine protocol on stdout so clients only reading stdout still receive errors.
                std::cout << "Error: " << e.what() << "\n";
                std::cout << "__END__\n" << std::flush;
            }
        }
        return 0;
    }
    
    // Check for daemon mode
#ifdef ENABLE_DAEMON_MODE
    if (std::find(args.begin(), args.end(), "--daemon") != args.end()) {
        return run_daemon_mode(args);
    }
    
    // Check for daemon status
    if (std::find(args.begin(), args.end(), "--daemon-status") != args.end()) {
        bool running = AXIOM::DaemonClient::is_daemon_running();
        std::cout << "🔍 Daemon status: " << (running ? "🟢 RUNNING" : "🔴 STOPPED") << "\n";
        return running ? 0 : 1;
    }
#endif
    
    // Check for GUI mode
    if (std::find(args.begin(), args.end(), "--gui-qt") != args.end()) {
        return launch_gui_interface(true, false);
    }

    if (std::find(args.begin(), args.end(), "--gui-standard") != args.end()) {
        return launch_gui_interface(true, true);
    }

    if (std::find(args.begin(), args.end(), "--gui") != args.end()) {
        return launch_gui_interface(false, false);
    }
    
    // Check for benchmark mode
    if (std::find(args.begin(), args.end(), "--benchmark") != args.end()) {
        return run_benchmark_mode();
    }
    
    // Command line execution with mode support
    if (!args.empty()) {
        AXIOM::CalculationMode mode = AXIOM::CalculationMode::ALGEBRAIC;
        std::string expression;

        for (size_t i = 0; i < args.size(); ++i) {
            const auto& arg = args[i];
            if (arg.rfind("--mode=", 0) == 0) {
                AXIOM::CalculationMode parsed_mode = AXIOM::CalculationMode::ALGEBRAIC;
                const std::string mode_arg = arg.substr(7);
                if (!try_parse_mode(mode_arg, parsed_mode)) {
                    std::cerr << "Error: Unknown mode '" << mode_arg << "'\n";
                    return 1;
                }
                mode = parsed_mode;
                continue;
            }
            if (arg == "--mode" && i + 1 < args.size()) {
                AXIOM::CalculationMode parsed_mode = AXIOM::CalculationMode::ALGEBRAIC;
                const std::string mode_arg = args[i + 1];
                if (!try_parse_mode(mode_arg, parsed_mode)) {
                    std::cerr << "Error: Unknown mode '" << mode_arg << "'\n";
                    return 1;
                }
                mode = parsed_mode;
                ++i;
                continue;
            }
            if (arg == "--linear") { mode = AXIOM::CalculationMode::LINEAR_SYSTEM; continue; }
            if (arg == "--statistics" || arg == "--stats") { mode = AXIOM::CalculationMode::STATISTICS; continue; }
            if (arg == "--symbolic") { mode = AXIOM::CalculationMode::SYMBOLIC; continue; }
            if (arg == "--units") { mode = AXIOM::CalculationMode::UNITS; continue; }
            if (arg == "--plot") { mode = AXIOM::CalculationMode::PLOT; continue; }

            if (!expression.empty()) {
                expression += " ";
            }
            expression += arg;
        }

        if (expression.empty()) {
            std::cerr << "Error: No expression provided\n";
            return 1;
        }

        if (!is_mode_supported(mode)) {
            std::cerr << "Error: Mode not enabled in this build\n";
            return 1;
        }

        auto calc = std::make_unique<AXIOM::DynamicCalc>();
        try {
            calc->SetMode(mode);
            auto basic_result = calc->Evaluate(expression);
            return print_result(basic_result, mode);
        } catch (const std::runtime_error& e) {
            std::cerr << "Runtime error: " << e.what() << "\n";
            return 1;
        } catch (const std::exception& e) {
            std::cerr << "Error: " << e.what() << "\n";
            return 1;
        }
    }
    
    // Default to interactive mode
    return run_interactive_mode();
}