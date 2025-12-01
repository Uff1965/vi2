#include "header.h"

#define PY_SSIZE_T_CLEAN
#include <Python.h>

#include <cstdio>

namespace python
{
	constexpr char script[] =
		"def Fib(n):\n"
		"    if n < 2:\n"
		"        return n\n"
		"    return Fib(n-1) + Fib(n-2)\n"

		"import embedded_cpp\n"
		"def Worker(msg, val):\n"
		"    result = embedded_cpp.callback(msg, val + 777)\n"
		"    return result\n";

	// Native C++ function exposed to Python
	PyObject* callback(PyObject*, PyObject* args)
	{   TM("0: Py callback");
		const char* message;
		int value;
		if (!PyArg_ParseTuple(args, "si", &message, &value))
		{	assert(false);
			return nullptr; // Argument parsing failed
		}

		int res = -1;
		if (const auto len = message ? strlen(message) : 0; len > 0)
		{	res = message[((value - KEY) % len + len) % len];
		}

		return PyLong_FromLong(res);
	}

	// Step 1: Initialize Python interpreter and register module
	bool init()
	{	TM("1: Py Initialize");

		// Module definition for "embedded_cpp"
		static PyMethodDef EmbMethods[] = { { "callback", callback, METH_VARARGS, "C++ Callback" }, {} };
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
		return PyRun_SimpleString(script) == 0;
	}

	int call_worker(const char* msg, int val)
	{	PyObject *main_module = PyImport_ImportModule("__main__");
		if (!main_module)
		{	assert(false);
			PyErr_Print();
			return -1;
		}

		PyObject *func = PyObject_GetAttrString(main_module, "Worker");
		if (!func)
		{	assert(false);
			PyErr_Print();
			Py_DECREF(main_module);
			return -1;
		}

		long result = -1;
		if (PyCallable_Check(func))
		{	PyObject *args = Py_BuildValue("(si)", msg, val);
			assert(args);
			if (PyObject *res = PyObject_CallObject(func, args))
			{	result = PyLong_AsLong(res);
				if (result == -1 && PyErr_Occurred())
				{	assert(false);
					PyErr_Print();
				}
				Py_DECREF(res);
			}
			else
			{	assert(false);
				PyErr_Print();
			}
			Py_XDECREF(args);
		}
		else
		{	assert(false);
			fprintf(stderr, "Python error: Worker is not callable\n");
		}

		Py_DECREF(func);
		Py_DECREF(main_module);
		return static_cast<int>(result);
	}

	int call_fibonacci(int val)
	{	PyObject *main_module = PyImport_ImportModule("__main__");
		if (!main_module)
		{	assert(false);
			PyErr_Print();
			return -1;
		}

		PyObject *func = PyObject_GetAttrString(main_module, "Fib");
		if (!func)
		{	assert(false);
			PyErr_Print();
			Py_DECREF(main_module);
			return -1;
		}

		long result = -1;
		if (PyCallable_Check(func))
		{	PyObject *args = Py_BuildValue("(i)", val);
			assert(args);
			if (PyObject *res = PyObject_CallObject(func, args))
			{	result = PyLong_AsLong(res);
				if (result == -1 && PyErr_Occurred())
				{	assert(false);
					PyErr_Print();
				}
				Py_DECREF(res);
			}
			else
			{	assert(false);
				PyErr_Print();
			}
			Py_XDECREF(args);
		}
		else
		{	assert(false);
			fprintf(stderr, "Python error: Worker is not callable\n");
		}

		Py_DECREF(func);
		Py_DECREF(main_module);
		return static_cast<int>(result);
	}

	// Step 3: Call Python function
	bool call()
	{
		{	TM("3.1: Py First Call");
			if (MSG[0] != call_worker(MSG, 0))
			{	assert(false);
				return false;
			}
		}

		for(int n = 0; n < 100; ++n)
		{	TM("3.2: Py Other Call");
			if (MSG[n % (strlen(MSG))] != call_worker(MSG, n))
			{	assert(false);
				return false;
			}
		}

		{	TM("3.3: Py Fib Call");
			if (const auto f = call_fibonacci(FIB_N); FIB_R != f)
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
	{	TM("*PYTHON test");

		bool result = false;
		if (init())
		{	if (load_script())
			{	result = call();
			}
			cleanup();
		}
		return result;
	}

	const auto _ = register_test("PYTHON", test);

} // namespace python
