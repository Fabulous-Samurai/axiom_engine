import subprocess
import sys
import threading
import time


class CppEngineInterface:
    """C++ engine with persistent subprocess (eliminates cold-start overhead)."""

    def __init__(self, executable_path: str | None):
        self.executable_path = executable_path
        self.failure_count = 0
        self.max_failures = 3
        self.current_mode = "algebraic"
        self.process = None
        self.lock = threading.Lock()
        self.use_persistent = True
        
        # Start persistent engine
        if self.use_persistent and executable_path:
            self._start_persistent_engine()

    def set_mode(self, mode: str) -> None:
        """Set the calculation mode for the engine."""
        self.current_mode = mode.lower()

    def close(self) -> None:
        """Cleanup persistent engine process."""
        if self.process:
            try:
                self.process.stdin.write("exit\n")
                self.process.stdin.flush()
                self.process.wait(timeout=1.0)
            except Exception:
                pass
            try:
                self.process.terminate()
                self.process.wait(timeout=1.0)
            except Exception:
                pass
            try:
                self.process.kill()
            except Exception:
                pass
            self.process = None

    def _start_persistent_engine(self) -> None:
        """Start persistent C++ engine subprocess."""
        if not self.executable_path:
            return
        
        try:
            startupinfo = None
            creationflags = 0
            if sys.platform == "win32":
                startupinfo = subprocess.STARTUPINFO()
                startupinfo.dwFlags |= subprocess.STARTF_USESHOWWINDOW
                startupinfo.wShowWindow = subprocess.SW_HIDE
                creationflags = subprocess.CREATE_NO_WINDOW
            
            # Start engine in interactive mode
            self.process = subprocess.Popen(
                [self.executable_path, "--interactive"],
                stdin=subprocess.PIPE,
                stdout=subprocess.PIPE,
                stderr=subprocess.PIPE,
                text=True,
                encoding="utf-8",
                errors="ignore",
                bufsize=0,  # Unbuffered for immediate I/O
                creationflags=creationflags,
                startupinfo=startupinfo,
            )
            # Give it a moment to initialize
            time.sleep(0.05)
        except Exception as e:
            print(f"Failed to start persistent engine: {e}")
            self.process = None
            self.use_persistent = False

    def _prepare_process_params(self) -> tuple:
        """Prepare subprocess parameters for Windows."""
        startupinfo = None
        creationflags = 0
        if sys.platform == "win32":
            startupinfo = subprocess.STARTUPINFO()
            startupinfo.dwFlags |= subprocess.STARTF_USESHOWWINDOW
            startupinfo.wShowWindow = subprocess.SW_HIDE
            creationflags = subprocess.CREATE_NO_WINDOW
        return startupinfo, creationflags

    def _check_result_errors(self, result_text: str) -> dict:
        """Check if result contains error messages."""
        if any(msg in result_text.lower() for msg in ["error:", "failed", "invalid"]):
            return {"success": False, "error": result_text, "fallback_needed": True}
        return {}

    def _mode_flag(self) -> str | None:
        """Return CLI mode flag for the configured engine mode."""
        mode = self.current_mode.lower()
        mapping = {
            "algebraic": None,
            "linear": "--mode=linear",
            "linear system": "--mode=linear",
            "statistics": "--mode=statistics",
            "stats": "--mode=statistics",
            "symbolic": "--mode=symbolic",
            "units": "--mode=units",
            "plot": "--mode=plot",
            "plotting": "--mode=plot",
        }
        return mapping.get(mode)

    def _build_command_args(self, command: str) -> list[str]:
        """Build axiom.exe CLI invocation with mode flag if needed."""
        args = [self.executable_path]
        mode_flag = self._mode_flag()
        if mode_flag:
            args.append(mode_flag)
        args.append(command)
        return args

    def _build_success_response(self, result_text: str, execution_time: float) -> dict:
        """Build successful response dictionary."""
        self.failure_count = 0
        return {
            "success": True,
            "result": result_text,
            "execution_time": round(execution_time, 1),
            "senna_speed": execution_time < 20,  # Persistent mode target
            "f1_speed": execution_time < 50,
            "persistent": True,
        }

    def _execute_persistent(self, command: str) -> dict:
        """Execute via persistent subprocess (FAST - no cold-start!)."""
        with self.lock:
            try:
                start_time = time.time()
                
                # Send mode change if needed
                mode = self.current_mode.lower()
                if mode and mode != "algebraic":
                    self.process.stdin.write(f":mode {mode}\n")
                    self.process.stdin.flush()
                
                # Send command
                self.process.stdin.write(command + "\n")
                self.process.stdin.flush()
                
                # Read result lines until __END__ marker or timeout
                result_lines = []
                timeout_time = time.time() + 3.0
                
                while time.time() < timeout_time:
                    # Check if process died
                    if self.process.poll() is not None:
                        raise Exception("Engine process terminated unexpectedly")
                    
                    # Read with short timeout (Windows compatible)
                    line = self.process.stdout.readline()
                    if not line:
                        time.sleep(0.01)  # Brief wait before retry
                        continue
                    
                    line = line.rstrip()
                    if line == "__END__":
                        break
                    
                    if line:  # Skip empty lines
                        result_lines.append(line)
                
                execution_time = (time.time() - start_time) * 1000
                result_text = "\n".join(result_lines).strip()
                
                if result_text:
                    error_response = self._check_result_errors(result_text)
                    if error_response:
                        return error_response
                    return self._build_success_response(result_text, execution_time)
                
                return {"success": False, "error": "Empty result", "fallback_needed": True}
                
            except Exception as e:
                print(f"Persistent engine error: {e}")
                # Mark for restart
                self.close()
                return {"success": False, "error": str(e), "fallback_needed": True}

    def execute_command(self, command: str) -> dict:
        """Execute command (persistent mode when available, fallback otherwise)."""
        if not self.executable_path:
            return {"success": False, "error": "C++ engine not available", "fallback_needed": True}

        # Try persistent mode first
        if self.use_persistent and self.process and self.process.poll() is None:
            result = self._execute_persistent(command)
            if result["success"] or not result.get("fallback_needed"):
                return result
            # Persistent failed, try to restart
            self._start_persistent_engine()
            if self.process and self.process.poll() is None:
                result = self._execute_persistent(command)
                if result["success"]:
                    return result

        # Fallback to single-shot mode
        process = None
        try:
            start_time = time.time()
            startupinfo, creationflags = self._prepare_process_params()

            process = subprocess.Popen(
                self._build_command_args(command),
                stdout=subprocess.PIPE,
                stderr=subprocess.PIPE,
                text=True,
                encoding="utf-8",
                errors="ignore",
                creationflags=creationflags,
                startupinfo=startupinfo,
            )

            stdout, stderr = process.communicate(timeout=3.0)
            execution_time = (time.time() - start_time) * 1000

            if process.returncode != 0:
                error_msg = stderr.strip() if stderr.strip() else "Command execution failed"
                return {"success": False, "error": error_msg, "fallback_needed": True}

            result_text = stdout.strip()
            if result_text:
                error_response = self._check_result_errors(result_text)
                if error_response:
                    return error_response
                response = self._build_success_response(result_text, execution_time)
                response["persistent"] = False
                return response
            
            return {"success": False, "error": "Empty result from engine", "fallback_needed": True}

        except subprocess.TimeoutExpired:
            self.failure_count += 1
            if process:
                try:
                    process.kill()
                except OSError:
                    pass
            return {"success": False, "error": "C++ engine timeout (3s)", "fallback_needed": True}
        except FileNotFoundError:
            return {"success": False, "error": f"Engine executable not found: {self.executable_path}", "fallback_needed": True}
        except Exception as exc:
            self.failure_count += 1
            return {"success": False, "error": f"Engine error: {str(exc)}", "fallback_needed": True}


    def execute_command_async(self, command: str, callback):
        """Execute command in background thread and invoke callback with result."""
        def worker():
            result = self.execute_command(command)
            callback(result)
        thread = threading.Thread(target=worker, daemon=True)
        thread.start()


class ResultCache:
    def __init__(self, max_size: int = 100):
        self.cache = {}
        self.max_size = max_size
        self.hits = 0
        self.misses = 0

    def get(self, key: str):
        if key in self.cache:
            self.hits += 1
            result = self.cache[key].copy()
            result["cached"] = True
            return result
        self.misses += 1
        return None

    def put(self, key: str, value: dict) -> None:
        if len(self.cache) >= self.max_size:
            self.cache.pop(next(iter(self.cache)))
        self.cache[key] = value.copy()

    def get_stats(self) -> str:
        total = self.hits + self.misses
        hit_rate = (self.hits / total * 100) if total > 0 else 0
        return f"Cache: {len(self.cache)} items | {hit_rate:.1f}% hit rate"


class PerformanceMonitor:
    def __init__(self):
        from collections import deque

        self.command_times = deque(maxlen=100)
        self.total_commands = 0

    def record(self, duration_ms: float) -> None:
        self.command_times.append(duration_ms)
        self.total_commands += 1

    def get_stats(self) -> str:
        if not self.command_times:
            return "No data"
        avg = sum(self.command_times) / len(self.command_times)
        return f"Avg: {avg:.1f}ms | Min: {min(self.command_times):.1f}ms | Max: {max(self.command_times):.1f}ms"

    def get_avg(self) -> float:
        if not self.command_times:
            return 0
        return sum(self.command_times) / len(self.command_times)


class CommandHistory:
    def __init__(self, max_size: int = 100):
        self.history: list[str] = []
        self.max_size = max_size
        self.position = 0

    def add(self, command: str) -> None:
        if command and (not self.history or self.history[-1] != command):
            self.history.append(command)
            if len(self.history) > self.max_size:
                self.history.pop(0)
        self.position = len(self.history)

    def prev(self):
        if self.position > 0:
            self.position -= 1
            return self.history[self.position]
        return None

    def next(self):
        if self.position < len(self.history) - 1:
            self.position += 1
            return self.history[self.position]
        if self.position == len(self.history) - 1:
            self.position = len(self.history)
            return ""
        return None

    def search(self, prefix: str):
        return [cmd for cmd in reversed(self.history) if cmd.startswith(prefix)]

    def get_all(self):
        return list(reversed(self.history))
