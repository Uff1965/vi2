#include "header.h"

#define PY_SSIZE_T_CLEAN
#include <Python.h>

#include <cstdio>

namespace python
{
	constexpr int VAL = 42;

	// Native C++ function exposed to Python
	// This simulates a callback from Python into C++.
	PyObject* cpp_callback(PyObject*, PyObject* args)
	{   TM("0: Py callback");
		const char* message;
		if (!PyArg_ParseTuple(args, "s", &message))
		{	assert(false);
			return nullptr; // Argument parsing failed
		}
		if (strcmp(message, "Hello, World!") != 0)
		{	assert(false);
			printf("Python callback: unexpected string '%s'\n", message);
		}
		return PyLong_FromLong(VAL); // Dummy return value for benchmarking
	}

	// Step 1: Initialize Python interpreter and register module
	bool init()
	{	TM("1: Py Initialize");

		// Module definition for "embedded_cpp"
		static PyMethodDef EmbMethods[] = { { "callback", cpp_callback, METH_VARARGS, "C++ Callback" }, {} };
		static PyModuleDef embmodule = { PyModuleDef_HEAD_INIT, "embedded_cpp", nullptr, -1, EmbMethods };
		if (PyImport_AppendInittab("embedded_cpp", [] { return PyModule_Create(&embmodule); }) == -1)
		{	assert(false);
			return false;
		}
		Py_Initialize();
		return true;
	}

	// Step 2: Load and run inline Python script
	bool load_script()
	{	TM("2: Py run");
		static constexpr char script[] =
			"import embedded_cpp\n"
			"def py_worker():\n"
			"\tresult = embedded_cpp.callback('Hello, World!')\n"
			"\treturn result\n";

		return PyRun_SimpleString(script) == 0;
	}

	// Step 3: Call Python function
	bool call_worker()
	{	PyObject *main_module = PyImport_ImportModule("__main__");
		if (!main_module)
		{	assert(false);
			PyErr_Print();
			return false;
		}

		PyObject *func = PyObject_GetAttrString(main_module, "py_worker");
		if (!func)
		{	assert(false);
			PyErr_Print();
			Py_DECREF(main_module);
			return false;
		}

		bool result = false;
		if (PyCallable_Check(func))
		{	if (PyObject *res = PyObject_CallObject(func, nullptr))
			{	Py_DECREF(res);
				const long val = PyLong_AsLong(res);
				if (val == -1 && PyErr_Occurred())
				{	assert(false);
					PyErr_Print();
				}
				else
				{	assert(val == VAL);
					result = true;
				}
			}
			else
			{	assert(false);
				PyErr_Print();
			}
		}
		else
		{	assert(false);
			fprintf(stderr, "Python error: py_worker is not callable\n");
		}

		Py_DECREF(func);
		Py_DECREF(main_module);

		return result;
	}

	bool call()
	{
		{	TM("3.1: Py First Call");
			if(!call_worker())
			{	assert(false);
				return false;
			}
		}

		for(int n = 0; n < 100; ++n)
		{	TM("3.2: Py Other Call");
			if(!call_worker())
			{	assert(false);
				return false;
			}
		}

		return true;
	}

	// Step 4: Finalize Python interpreter
	void cleanup()
	{	TM("4: Py Cleanup");
		if (int res = Py_FinalizeEx())
		{	assert(false);
			fprintf(stderr, "Python finalization failed (code %d)\n", res);
		}
	}

	// Test entry
	bool test()
	{	TM("python test");

		bool result = false;
		if (init())
		{	if (load_script())
			{	result = call();
			}
			cleanup();
		}
		return result;
	}

	const auto _ = register_test("Python", test);

} // namespace python
