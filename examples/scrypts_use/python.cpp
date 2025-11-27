#include "header.h"
#include <vi_timing/vi_timing.h>

#define PY_SSIZE_T_CLEAN
#include <Python.h>

namespace python
{
	// Native C++ function exposed to Python
	// This simulates a callback from Python into C++.
	PyObject* cpp_callback(PyObject*, PyObject* args)
	{   TM("0: Py callback");
		const char* message;
		if (!PyArg_ParseTuple(args, "s", &message))
		{	return nullptr; // Argument parsing failed
		}
		return PyLong_FromLong(42); // Return a dummy integer
	}

	// Step 1: Initialize Python interpreter and register module
	bool init()
	{	TM("1: Py Initialize");

		// Module definition for "embedded_cpp"
		static PyMethodDef EmbMethods[] = { { "callback", cpp_callback, METH_VARARGS, "C++ Callback" }, {} };
		static struct PyModuleDef embmodule = { PyModuleDef_HEAD_INIT, "embedded_cpp", NULL, -1, EmbMethods };
		if (PyImport_AppendInittab("embedded_cpp", [] { return PyModule_Create(&embmodule); }) == -1)
		{	return false;
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
			"\tresult = embedded_cpp.callback('Hello from Python!')\n";

		//return PyRun_SimpleString(script) == 0;

		{	bool result = false;
			PyObject *code_obj = nullptr;
			{	TM("2.1: Py compile");
				code_obj = Py_CompileString(script, "<string>", Py_file_input);
				if (!code_obj)
				{	PyErr_Print();
				}
			}
			if (code_obj)
			{	TM("2.2: Py eval");
				if (PyObject *main_module = PyImport_AddModule("__main__"))
				{	if (PyObject *globals = PyModule_GetDict(main_module))
					{	if (PyObject *evalcode = PyEval_EvalCode(code_obj, globals, globals))
						{	result = true;
							Py_DECREF(evalcode);
						}
						else
						{	PyErr_Print();
						}
					}
					else
					{	PyErr_Print();
					}
				}
			}
			Py_DECREF(code_obj);
			return result;
		}
	}

	// Step 3: Call Python function
	bool call_worker()
	{	PyObject* main_module = PyImport_ImportModule("__main__");
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

	bool call()
	{
		{	TM("3.1: Py First Call");
			if(!call_worker())
			{	return false;
			}
		}

		for(int n = 0; n < 100; ++n)
		{	TM("3.2: Py Other Call");
			if(!call_worker())
			{	return false;
			}
		}

		return true;
	}

	// Step 4: Finalize Python interpreter
	void cleanup()
	{	TM("4: Py Cleanup");
		Py_Finalize();
	}

	// Test entry
	void test()
	{	TM("python test");

		if (init())
		{	if (load_script())
			{	call();
			}
			cleanup();
		}
	}

	const auto _ = register_test(test);

} // namespace python
