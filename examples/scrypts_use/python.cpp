#include "header.h"

#include <vi_timing/vi_timing.h>

#define PY_SSIZE_T_CLEAN
#include <Python.h>

VI_TM(FILE_PATH);

namespace python
{
	// 1. C++ Function to be called from Python
	PyObject *emb_callback(PyObject *, PyObject *args)
	{	VI_TM_FUNC;
		const char *message;
		if (!PyArg_ParseTuple(args, "s", &message))
		{	return nullptr;
		}
		return PyLong_FromLong(1); // Return an integer (e.g., status code 1)
	}

	// 2. Module Definition
	static PyMethodDef EmbMethods[] = { { "callback", emb_callback, METH_VARARGS, "C++ Callback" }, {} };
	static struct PyModuleDef embmodule = { PyModuleDef_HEAD_INIT, "embedded_cpp", NULL, -1, EmbMethods };
	static PyObject *PyInit_embedded_cpp(void)
	{	return PyModule_Create(&embmodule);
	}

	// 3. Main Application
	void test_python(void)
	{	VI_TM_FUNC;

		{	VI_TM("Py(1) Initialize");
			// Register the "embedded_cpp" module before initializing Python
			if (PyImport_AppendInittab("embedded_cpp", &PyInit_embedded_cpp) == -1)
			{	return;
			}

			Py_Initialize(); // Initialize the Python Interpreter
		}

		{	VI_TM("Py(2) Python run");
			// In a real app, you might load a .py file. 
			// Here we define the script inline for simplicity.
			const char *py_script =
				"import embedded_cpp\n"
				"def py_worker():\n"
				"	result = embedded_cpp.callback('Hello from Python!')\n";

			// Run the string to define the function in __main__
			if (PyRun_SimpleString(py_script) != 0)
			{
				Py_Finalize();
				return;
			}
		}

		PyObject *main_module = nullptr;
		PyObject *func = nullptr;

		{	VI_TM("Py(3) Call");
			// Import __main__ module to access the function we just defined
			main_module = PyImport_ImportModule("__main__");
			func = PyObject_GetAttrString(main_module, "py_worker");

			if (func && PyCallable_Check(func))
			{	// --- THE FULL CYCLE HAPPENS HERE ---
				PyObject *result = PyObject_CallObject(func, NULL);

				if (result != NULL)
				{	Py_DECREF(result);
				}
				else
				{	PyErr_Print();
				}
			}
			else if (PyErr_Occurred())
			{
				PyErr_Print();
			}
		}

		{	VI_TM("Py(4) Cleanup");
			// Cleanup
			Py_XDECREF(func);
			func = nullptr;
			Py_XDECREF(main_module);
			main_module = nullptr;

			Py_Finalize();
		}
		return;
	}

	const auto _ = register_test(test_python);

} // namespace python
