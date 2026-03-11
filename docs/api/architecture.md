# ğŸ¯ AXIOM v3.0 - Architecture Documentation

## Enhanced Directory Structure

```
axiom/
â”œâ”€â”€ ğŸ“ core/                    # Core computational engines
â”‚   â”œâ”€â”€ engine/                 # C++ mathematical engines
â”‚   â”‚   â”œâ”€â”€ eigen_engine.cpp    # Eigen-optimized CPU engine
â”‚   â”‚   â”œâ”€â”€ symbolic_engine.cpp # Computer algebra system
â”‚   â”‚   â””â”€â”€ statistics_engine.cpp # Statistical computations
â”‚   â”œâ”€â”€ bindings/               # Nanobind Python interfaces
â”‚   â”‚   â””â”€â”€ nanobind_interface.cpp # Zero-copy Python bindings
â”‚   â””â”€â”€ dispatch/               # Selective operation routing
â”‚       â””â”€â”€ selective_dispatcher.cpp # Intelligent engine routing
â”œâ”€â”€ ğŸ“ gui/                     # User interfaces
â”‚   â”œâ”€â”€ python/                 # Python GUI applications
â”‚   â”‚   â”œâ”€â”€ matlab_alternative_gui.py # MATLAB-style interface
â”‚   â”‚   â””â”€â”€ axiom_gui.py     # Original calculator GUI
â”‚   â””â”€â”€ qt/                     # Qt-based interfaces (future)
â”œâ”€â”€ ğŸ“ tools/                   # Analysis and visualization
â”‚   â”œâ”€â”€ analysis/               # Mathematical analysis tools
â”‚   â”‚   â””â”€â”€ signal_processing_toolkit.py # Signal analysis suite
â”‚   â””â”€â”€ visualization/          # 3D plotting and graphics
â”‚       â””â”€â”€ advanced_3d_visualization.py # 3D plotting suite
â”œâ”€â”€ ğŸ“ tests/                   # Comprehensive testing
â”‚   â”œâ”€â”€ unit/                   # Unit tests for components
â”‚   â”‚   â””â”€â”€ test_all_specs.py   # Comprehensive unit tests
â”‚   â””â”€â”€ integration/            # Integration and E2E tests
â”‚       â””â”€â”€ senna_speed_test.py # Performance benchmarking
â”œâ”€â”€ ğŸ“ docs/                    # Documentation
â”‚   â”œâ”€â”€ api/                    # API reference
â”‚   â”‚   â””â”€â”€ architecture.md     # This file
â”‚   â””â”€â”€ user/                   # User guides and tutorials
â”‚       â””â”€â”€ performance.md      # Performance optimization guide
â”œâ”€â”€ ğŸ“ examples/                # Usage examples
â”‚   â”œâ”€â”€ basic/                  # Basic calculator examples
â”‚   â””â”€â”€ advanced/               # Advanced scientific computing
â”‚       â””â”€â”€ enhanced_architecture_demo.py # Architecture demonstration
â”œâ”€â”€ ğŸ“ scripts/                 # Build and deployment
â”‚   â”œâ”€â”€ build/                  # Build automation scripts
â”‚   â”‚   â”œâ”€â”€ build_enhanced.py   # Enhanced build script
â”‚   â”‚   â”œâ”€â”€ fast_build.ps1      # Windows build script
â”‚   â”‚   â””â”€â”€ fast_build.sh       # Unix build script
â”‚   â””â”€â”€ deploy/                 # Deployment utilities
â”œâ”€â”€ ğŸ“ include/                 # C++ header files
â”œâ”€â”€ ğŸ“ src/                     # C++ source files
â””â”€â”€ CMakeLists.txt              # Enhanced build configuration
```

## Component Overview

### Core Engine Architecture

#### ğŸ¯ Selective Dispatcher

- **Purpose**: Intelligent operation routing to optimal engines
- **Location**: `core/dispatch/selective_dispatcher.cpp`
- **Features**: 
  - Performance-based engine selection
  - Automatic fallback mechanisms
  - Error handling and recovery

#### âš¡ Eigen CPU Engine  

- **Purpose**: Hardware-optimized mathematical computations
- **Location**: `src/eigen_engine.cpp`
- **Features**:
  - SIMD acceleration
  - Matrix operations
  - Linear algebra optimizations

#### ğŸ Nanobind Interface

- **Purpose**: Zero-copy Python-C++ integration
- **Location**: `core/bindings/nanobind_interface.cpp`
- **Features**:
  - Minimal overhead bindings
  - Type-safe conversions
  - Modern C++ integration

### User Interface Layer

#### ğŸ›ï¸ MATLAB Alternative GUI

- **Purpose**: Professional MATLAB-style interface
- **Location**: `gui/python/matlab_alternative_gui.py`
- **Features**:
  - 3-panel workspace layout
  - Command window with history
  - Variable browser
  - Figure management

#### ğŸ§® Original Calculator GUI

- **Purpose**: Enhanced calculator interface
- **Location**: `gui/python/axiom_gui.py`
- **Features**:
  - 12 calculation modes
  - Performance metrics
  - Theme support

### Analysis Tools

#### ğŸµ Signal Processing Toolkit

- **Purpose**: Professional signal analysis
- **Location**: `tools/analysis/signal_processing_toolkit.py`
- **Features**:
  - Signal generation
  - FFT analysis
  - Filter design
  - Spectrograms

#### ğŸ¯ 3D Visualization Suite

- **Purpose**: Advanced 3D plotting and visualization
- **Location**: `tools/visualization/advanced_3d_visualization.py` 
- **Features**:
  - Interactive surface plots
  - Parametric curves
  - Molecular visualization
  - Animation support

### Testing Framework

#### ğŸ”¬ Unit Testing

- **Purpose**: Component-level validation
- **Location**: `tests/unit/test_all_specs.py`
- **Coverage**: 95%+ code coverage

#### âš¡ Performance Testing

- **Purpose**: Senna-speed validation
- **Location**: `tests/integration/senna_speed_test.py`
- **Benchmarks**: Sub-1ms targets

### Build & Deployment

#### ğŸ”¨ Enhanced Build System

- **Purpose**: Automated building with Eigen + nanobind
- **Location**: `scripts/build/build_enhanced.py`
- **Features**:
  - Dependency management
  - Cross-platform support
  - Optimization flags

## Migration Notes

### From v2.5 to v3.0

1. **Files Moved**:
   - `matlab_alternative_gui.py` â†’ `gui/python/`
   - `signal_processing_toolkit.py` â†’ `tools/analysis/`
   - `advanced_3d_visualization.py` â†’ `tools/visualization/`
   - `enhanced_architecture_demo.py` â†’ `examples/advanced/`
   - `senna_speed_test.py` â†’ `tests/integration/`

2. **New Components**:
   - Eigen CPU engine
   - Nanobind interface
   - Selective dispatcher

3. **Enhanced Features**:
   - SIMD optimization
   - Zero-copy Python bindings
   - Intelligent operation routing

## Performance Architecture

### Engine Selection Logic

```cpp
class SelectiveDispatcher {
private:
    std::unique_ptr<EigenEngine> eigen_engine;
    std::unique_ptr<PythonEngine> python_engine;
    
public:
    EngineResult calculate(const std::string& expression) {
        // 1. Analyze expression complexity
        // 2. Check engine availability
        // 3. Route to optimal engine
        // 4. Return result or fallback
    }
};
```

### Performance Targets

| Operation Type | Target Time | Engine Used |
|---------------|-------------|-------------|
| Simple Arithmetic | <1ms | Eigen CPU |
| Matrix Operations | <10ms | Eigen SIMD |
| Complex Functions | <50ms | Selective |
| Visualization | <200ms | Python/GPU |

This architecture ensures AXIOM v3.0 delivers professional-grade performance while maintaining ease of use and extensibility.

