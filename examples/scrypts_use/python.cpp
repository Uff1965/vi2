#include "header.h"
#include <vi_timing/vi_timing.h>

#define PY_SSIZE_T_CLEAN
#include <Python.h>

VI_TM(FILE_PATH);

namespace python
{
	// Native C++ function exposed to Python
	// This simulates a callback from Python into C++.
	PyObject* emb_callback(PyObject*, PyObject* args)
	{   VI_TM("0: Py callback");
		const char* message;
		if (!PyArg_ParseTuple(args, "s", &message))
		{	return nullptr; // Argument parsing failed
		}
		return PyLong_FromLong(1); // Return a dummy integer (status code)
	}

	// Module definition for "embedded_cpp"
	static PyMethodDef EmbMethods[] = { { "callback", emb_callback, METH_VARARGS, "C++ Callback" }, {} };
	static struct PyModuleDef embmodule = { PyModuleDef_HEAD_INIT, "embedded_cpp", NULL, -1, EmbMethods };
	static PyObject* PyInit_embedded_cpp(void)
	{	return PyModule_Create(&embmodule);
	}

	// Step 1: Initialize Python interpreter and register module
	bool init_python()
	{	VI_TM("1: Py Initialize");
		if (PyImport_AppendInittab("embedded_cpp", &PyInit_embedded_cpp) == -1)
		{	return false;
		}
		Py_Initialize();
		return true;
	}

	// Step 2: Load and run inline Python script
	bool load_script()
	{	VI_TM("2: Py Python run");
		static constexpr char py_script[] =
			"import embedded_cpp\n"
			"def py_worker():\n"
			"\tresult = embedded_cpp.callback('Hello from Python!')\n";
		if (PyRun_SimpleString(py_script) != 0)
		{	Py_Finalize();
			return false;
		}
		return true;
	}

	// Step 3: Call Python function
	bool call_worker()
	{
		VI_TM("3: Py Call");
		PyObject* main_module = PyImport_ImportModule("__main__");
		PyObject* func = PyObject_GetAttrString(main_module, "py_worker");

		if (func && PyCallable_Check(func))
		{
			PyObject* result = PyObject_CallObject(func, NULL);
			if (result != NULL)
			{
				Py_DECREF(result);
			}
			else
			{
				PyErr_Print();
			}
		}
		else if (PyErr_Occurred())
		{
			PyErr_Print();
		}

		Py_XDECREF(func);
		Py_XDECREF(main_module);
		return true;
	}

	// Step 4: Finalize Python interpreter
	void cleanup_python()
	{
		VI_TM("4: Py Cleanup");
		Py_Finalize();
	}

	// Main test function
	// Purpose: measure mandatory time costs of standard steps
	// when working with an embedded scripting language (Python).
	void test_python()
	{
		VI_TM_FUNC;
		if (!init_python()) return;
		if (load_script())
			call_worker();
		cleanup_python();
	}

	const auto _ = register_test(test_python);

} // namespace python
