#include "python_engine.h"
#include "string_helpers.h"
#include <iostream>
#include <sstream>
#include <algorithm>

// Python initialization and cleanup
PythonEngine::PythonEngine() : initialized_(false) {
    Initialize();
}

PythonEngine::~PythonEngine() {
    Finalize();
}

bool PythonEngine::Initialize() {
    if (initialized_) return true;

    try {
        // Initialize Python interpreter
        Py_Initialize();
        if (!Py_IsInitialized()) {
            last_error_ = "Failed to initialize Python interpreter";
            return false;
        }

        // Import sys to add paths if needed
        PyRun_SimpleString("import sys");
        
        // Try to import essential modules
        bool auto_import_numpy = true;
        bool auto_import_scipy = true;
        bool auto_import_matplotlib = true;
        
        if (auto_import_numpy) {
            PyRun_SimpleString("try:\n    import numpy as np\n    print('NumPy loaded successfully')\nexcept ImportError:\n    print('NumPy not available')");
        }
        
        if (auto_import_scipy) {
            PyRun_SimpleString("try:\n    import scipy\n    print('SciPy loaded successfully')\nexcept ImportError:\n    print('SciPy not available')");
        }
        
        if (auto_import_matplotlib) {
            PyRun_SimpleString("try:\n    import matplotlib.pyplot as plt\n    print('Matplotlib loaded successfully')\nexcept ImportError:\n    print('Matplotlib not available')");
        }

        initialized_ = true;
        return true;
    } catch (const std::exception& e) {
        last_error_ = std::string("Exception during Python initialization: ") + e.what();
        return false;
    } catch (...) {
        last_error_ = "Unknown exception during Python initialization";
        return false;
    }
}

void PythonEngine::Finalize() {
    if (!initialized_) return;

    // Clean up cached modules
    for (auto& [name, module] : cached_modules_) {
        Py_XDECREF(module);
    }
    cached_modules_.clear();

    // Finalize Python interpreter
    if (Py_IsInitialized()) {
        Py_Finalize();
    }
    
    initialized_ = false;
}

// Core execution functions
EngineResult PythonEngine::ExecutePython(const std::string& code) {
    if (!initialized_) {
        return {{}, {EngineErrorResult(CalcErr::OperationNotFound)}};
    }

    try {
        // Execute Python code
        int result = PyRun_SimpleString(code.c_str());
        
        if (result == 0) {
            return EngineSuccessResult(std::string("Python code executed successfully"));
        } else {
            SetErrorFromPython();
            return {{}, {EngineErrorResult(CalcErr::DomainError)}};
        }
    } catch (const std::exception& e) {
        last_error_ = std::string("Exception during Python execution: ") + e.what();
        return {{}, {EngineErrorResult(CalcErr::DomainError)}};
    } catch (...) {
        last_error_ = "Unknown exception during Python execution";
        return {{}, {EngineErrorResult(CalcErr::DomainError)}};
    }
}

EngineResult PythonEngine::EvaluatePython(const std::string& expression) {
    if (!initialized_) {
        return {{}, {EngineErrorResult(CalcErr::OperationNotFound)}};
    }

    try {
        // Create a Python string from the expression
        std::string eval_code = "result = " + expression;
        
        // Execute the assignment
        int exec_result = PyRun_SimpleString(eval_code.c_str());
        if (exec_result != 0) {
            SetErrorFromPython();
            return {{}, {EngineErrorResult(CalcErr::ParseError)}};
        }

        // Get the result from the global namespace
        PyObject* main_module = PyImport_AddModule("__main__");
        PyObject* global_dict = PyModule_GetDict(main_module);
        PyObject* result_obj = PyDict_GetItemString(global_dict, "result");
        
        if (!result_obj) {
            last_error_ = "Failed to get result from Python evaluation";
            return {{}, {EngineErrorResult(CalcErr::DomainError)}};
        }

        // Convert Python object to appropriate C++ type
        if (PyFloat_Check(result_obj) || PyLong_Check(result_obj)) {
            double value = PyFloat_AsDouble(result_obj);
            if (PyErr_Occurred()) {
                SetErrorFromPython();
                return {{}, {EngineErrorResult(CalcErr::DomainError)}};
            }
            return EngineSuccessResult(value);
        }
        else if (PyList_Check(result_obj)) {
            std::vector<double> vec = PyListToVector(result_obj);
            if (!last_error_.empty()) {
                return {{}, {EngineErrorResult(CalcErr::DomainError)}};
            }
            return EngineSuccessResult(vec);
        }
        else {
            // Convert to string representation
            std::string str_result = PyObjectToString(result_obj);
            return EngineSuccessResult(str_result);
        }

    } catch (const std::exception& e) {
        last_error_ = std::string("Exception during Python evaluation: ") + e.what();
        return {{}, {EngineErrorResult(CalcErr::DomainError)}};
    } catch (...) {
        last_error_ = "Unknown exception during Python evaluation";
        return {{}, {EngineErrorResult(CalcErr::DomainError)}};
    }
}

// Variable management
bool PythonEngine::SetVariable(const std::string& name, double value) {
    if (!initialized_) return false;

    try {
        PyObject* main_module = PyImport_AddModule("__main__");
        PyObject* global_dict = PyModule_GetDict(main_module);
        PyObject* py_value = PyFloat_FromDouble(value);
        
        int result = PyDict_SetItemString(global_dict, name.c_str(), py_value);
        Py_DECREF(py_value);
        
        return result == 0;
    } catch (const std::exception& e) {
        last_error_ = std::string("Exception setting variable: ") + e.what();
        return false;
    } catch (...) {
        last_error_ = "Unknown exception setting variable";
        return false;
    }
}

bool PythonEngine::SetVariable(const std::string& name, const std::vector<double>& values) {
    if (!initialized_) return false;

    try {
        PyObject* main_module = PyImport_AddModule("__main__");
        PyObject* global_dict = PyModule_GetDict(main_module);
        PyObject* py_list = VectorToPyList(values);
        
        int result = PyDict_SetItemString(global_dict, name.c_str(), py_list);
        Py_DECREF(py_list);
        
        return result == 0;
    } catch (const std::exception& e) {
        last_error_ = std::string("Exception setting vector variable: ") + e.what();
        return false;
    } catch (...) {
        last_error_ = "Unknown exception setting vector variable";
        return false;
    }
}

// NumPy integration
EngineResult PythonEngine::CreateNumpyArray(const std::vector<double>& data) {
    if (!initialized_) {
        return {{}, {EngineErrorResult(CalcErr::OperationNotFound)}};
    }

    try {
        // Set the data as a Python variable
        SetVariable("temp_data", data);
        
        // Create NumPy array
        int result = PyRun_SimpleString("import numpy as np; temp_array = np.array(temp_data)");
        
        if (result == 0) {
            return EngineSuccessResult(std::string("NumPy array created successfully"));
        } else {
            SetErrorFromPython();
            return {{}, {EngineErrorResult(CalcErr::DomainError)}};
        }
    } catch (const std::exception& e) {
        last_error_ = std::string("Exception during NumPy array creation: ") + e.what();
        return {{}, {EngineErrorResult(CalcErr::DomainError)}};
    } catch (...) {
        last_error_ = "Unknown exception during NumPy array creation";
        return {{}, {EngineErrorResult(CalcErr::DomainError)}};
    }
}

EngineResult PythonEngine::MatplotlibPlot(const std::string& expression, double x_min, double x_max, int points) {
    if (!initialized_) {
        return {{}, {EngineErrorResult(CalcErr::OperationNotFound)}};
    }

    try {
        // Create the plotting code
        std::stringstream plot_code;
        plot_code << "import numpy as np\n";
        plot_code << "import matplotlib.pyplot as plt\n";
        plot_code << "x = np.linspace(" << x_min << ", " << x_max << ", " << points << ")\n";
        plot_code << "y = " << expression << "\n";
        plot_code << "plt.figure(figsize=(10, 6))\n";
        plot_code << "plt.plot(x, y)\n";
        plot_code << "plt.grid(True)\n";
        plot_code << "plt.title('Plot of " << expression << "')\n";
        plot_code << "plt.xlabel('x')\n";
        plot_code << "plt.ylabel('y')\n";
        
        int result = PyRun_SimpleString(plot_code.str().c_str());
        
        if (result == 0) {
            return EngineSuccessResult(std::string("Plot created successfully. Use plt.show() to display."));
        } else {
            SetErrorFromPython();
            return {{}, {EngineErrorResult(CalcErr::DomainError)}};
        }
    } catch (const std::exception& e) {
        last_error_ = std::string("Exception during matplotlib plotting: ") + e.what();
        return {{}, {EngineErrorResult(CalcErr::DomainError)}};
    } catch (...) {
        last_error_ = "Unknown exception during matplotlib plotting";
        return {{}, {EngineErrorResult(CalcErr::DomainError)}};
    }
}

// Utility functions
PyObject* PythonEngine::VectorToPyList(const std::vector<double>& vec) {
    PyObject* list = PyList_New(vec.size());
    for (size_t i = 0; i < vec.size(); ++i) {
        PyObject* item = PyFloat_FromDouble(vec[i]);
        PyList_SetItem(list, i, item);
    }
    return list;
}

std::vector<double> PythonEngine::PyListToVector(PyObject* obj) {
    std::vector<double> result;
    
    if (!PyList_Check(obj)) {
        last_error_ = "Object is not a Python list";
        return result;
    }
    
    Py_ssize_t size = PyList_Size(obj);
    result.reserve(size);
    
    for (Py_ssize_t i = 0; i < size; ++i) {
        PyObject* item = PyList_GetItem(obj, i);
        if (PyFloat_Check(item) || PyLong_Check(item)) {
            result.push_back(PyFloat_AsDouble(item));
        } else {
            last_error_ = "List contains non-numeric values";
            return std::vector<double>();
        }
    }
    
    return result;
}

std::string PythonEngine::PyObjectToString(PyObject* obj) {
    if (!obj) return "None";
    
    PyObject* str_obj = PyObject_Str(obj);
    if (!str_obj) return "Error converting to string";
    
    const char* str_ptr = PyUnicode_AsUTF8(str_obj);
    std::string result = str_ptr ? str_ptr : "Error getting string";
    
    Py_DECREF(str_obj);
    return result;
}

void PythonEngine::SetErrorFromPython() {
    if (PyErr_Occurred()) {
        PyObject *ptype, *pvalue, *ptraceback;
        PyErr_Fetch(&ptype, &pvalue, &ptraceback);
        
        if (pvalue) {
            last_error_ = PyObjectToString(pvalue);
        } else {
            last_error_ = "Unknown Python error";
        }
        
        Py_XDECREF(ptype);
        Py_XDECREF(pvalue); 
        Py_XDECREF(ptraceback);
    }
}

EngineResult PythonEngine::GetPythonVersion() {
    if (!initialized_) {
        return {{}, {EngineErrorResult(CalcErr::OperationNotFound)}};
    }

    try {
        int result = PyRun_SimpleString("import sys; version_info = sys.version");
        if (result == 0) {
            return EvaluatePython("version_info");
        } else {
            return {{}, {EngineErrorResult(CalcErr::DomainError)}};
        }
    } catch (...) {
        return {{}, {EngineErrorResult(CalcErr::DomainError)}};
    }
}