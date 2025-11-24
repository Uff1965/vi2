#define PY_SSIZE_T_CLEAN
#include <Python.h>
#include <iostream>

// ---------------------------------------------------------
// 1. C++ Function to be called from Python
// ---------------------------------------------------------
static PyObject* emb_callback(PyObject* self, PyObject* args) {
    const char* message;
    // Parse arguments: expect a string ("s")
    if (!PyArg_ParseTuple(args, "s", &message)) {
        return NULL;
    }
    std::cout << "[C++] Callback received: " << message << std::endl;
    
    // Return an integer (e.g., status code 1)
    return PyLong_FromLong(1);
}

// ---------------------------------------------------------
// 2. Module Definition
// ---------------------------------------------------------
static PyMethodDef EmbMethods[] = {
    {"callback", emb_callback, METH_VARARGS, "Print a message from Python."},
    {NULL, NULL, 0, NULL}
};

static struct PyModuleDef embmodule = {
    PyModuleDef_HEAD_INIT, "embedded_cpp", NULL, -1, EmbMethods
};

static PyObject* PyInit_embedded_cpp(void) {
    return PyModule_Create(&embmodule);
}

// ---------------------------------------------------------
// 3. Main Application
// ---------------------------------------------------------
int main(int argc, char* argv[]) {
    std::cout << "[C++] Starting application..." << std::endl;

    // Register the "embedded_cpp" module before initializing Python
    if (PyImport_AppendInittab("embedded_cpp", &PyInit_embedded_cpp) == -1) {
        std::cerr << "Error: Could not extend in-built modules table" << std::endl;
        return 1;
    }

    // Initialize the Python Interpreter
    Py_Initialize();

    // In a real app, you might load a .py file. 
    // Here we define the script inline for simplicity.
    const char* py_script =
        "import embedded_cpp\n"
        "import sys\n"
        "\n"
        "def py_worker():\n"
        "    print('[Python] Inside py_worker function')\n"
        "    print('[Python] Calling C++ callback...')\n"
        "    result = embedded_cpp.callback('Hello from Python!')\n"
        "    print(f'[Python] Callback returned: {result}')\n"
        "    print('[Python] Finishing py_worker')\n";

    // Run the string to define the function in __main__
    if (PyRun_SimpleString(py_script) != 0) {
        std::cerr << "Error running Python script" << std::endl;
        Py_Finalize();
        return 1;
    }

    // Import __main__ module to access the function we just defined
    PyObject* main_module = PyImport_ImportModule("__main__");
    PyObject* func = PyObject_GetAttrString(main_module, "py_worker");

    if (func && PyCallable_Check(func)) {
        std::cout << "[C++] Calling Python function 'py_worker'..." << std::endl;
        
        // --- THE FULL CYCLE HAPPENS HERE ---
        PyObject* result = PyObject_CallObject(func, NULL);
        
        if (result != NULL) {
            std::cout << "[C++] Python function returned successfully." << std::endl;
            Py_DECREF(result);
        } else {
            PyErr_Print();
        }
    } else {
        if (PyErr_Occurred()) PyErr_Print();
        std::cerr << "Cannot find function 'py_worker'" << std::endl;
    }

    // Cleanup
    Py_XDECREF(func);
    Py_XDECREF(main_module);
    Py_Finalize();

    std::cout << "[C++] Exiting." << std::endl;
    return 0;
}