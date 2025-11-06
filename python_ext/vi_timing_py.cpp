#define PY_SSIZE_T_CLEAN

#include <vi_timing/vi_timing.h>

#include <Python.h>

#include <cassert>
#include <thread>

#ifdef _WIN32
    #define API_EXPORT __declspec(dllexport)
#else
  #define API_EXPORT __attribute__((visibility("default")))
#endif

extern "C"
{
	API_EXPORT
	void vi_tmDummy()
	{	/**/
	}

	API_EXPORT
	void vi_tmSleep(uint32_t milliseconds)
	{	std::this_thread::sleep_for(std::chrono::milliseconds(milliseconds));
	}

	API_EXPORT
	const char *vi_tmVersion()
	{	return static_cast<const char*>(vi_tmStaticInfo(vi_tmInfoVersion));
	}
} // extern "C"

namespace
{
	PyObject* vi_timing_dummy(PyObject* Py_UNUSED(self), PyObject* noargs)
	{	assert(nullptr == noargs);
		::vi_tmDummy();
		Py_RETURN_NONE;
	}

	PyObject* vi_timing_sleep(PyObject* Py_UNUSED(self), PyObject *arg)
	{	if (!PyLong_Check(arg))
		{	PyErr_SetString(PyExc_TypeError, "Argument must be an integer");
		}
		else if (const auto milliseconds = PyLong_AsUnsignedLong(arg); !PyErr_Occurred())
		{	::vi_tmSleep(milliseconds);
			Py_RETURN_NONE;
		}

		assert(false);
		return NULL;
	}

	PyObject* vi_timing_version(PyObject *Py_UNUSED(self), PyObject* noargs)
	{	assert(nullptr == noargs);
		auto str = ::vi_tmVersion();
		return PyUnicode_FromString(str);
	}

	PyObject* vi_timing_init(PyObject *Py_UNUSED(self), PyObject *args, PyObject *kwargs)
	{	static constexpr const char * kwlist[] = { "title", "report_flags", "flags", nullptr };
		const char *title;
		int report_flags;
		int flags;
		if (PyArg_ParseTupleAndKeywords(args, kwargs, "sii", kwlist, &title, &report_flags, &flags))
		{	if (long result = vi_tmInit(title, report_flags, flags); VI_SUCCESS(result))
			{	return PyLong_FromLong(result);
			}
			PyErr_SetString(PyExc_RuntimeError, "Failed to initialize timing library");
		}
		assert(false);
		return NULL;
	}

	PyObject* vi_timing_shutdown(PyObject* Py_UNUSED(self), PyObject* noargs)
	{	assert(nullptr == noargs);
		vi_tmShutdown();
		Py_RETURN_NONE;
	}

	PyObject* vi_timing_registry_create(PyObject* Py_UNUSED(self), PyObject* noargs)
	{	assert(nullptr == noargs);
		if (VI_TM_HJOUR jour = vi_tmRegistryCreate())
		{	return PyLong_FromVoidPtr(jour);
		}
		PyErr_SetString(PyExc_RuntimeError, "Failed to create registry");
		assert(false);
		return NULL;
	}

	PyObject* vi_timing_registry_close(PyObject* Py_UNUSED(self), PyObject* arg)
	{	if (auto jour = static_cast<VI_TM_HJOUR>(PyLong_AsVoidPtr(arg)); !PyErr_Occurred())
		{	vi_tmRegistryClose(jour);
			Py_RETURN_NONE;
		}
		assert(false);
		return NULL;
	}

	PyObject* vi_timing_registry_get_meas(PyObject* Py_UNUSED(self), PyObject* args, PyObject* kwargs)
	{	static constexpr const char* kwlist[] = {"jour", "name", NULL};
		PyObject* pobj;
		char* name;
		if (PyArg_ParseTupleAndKeywords(args, kwargs, "Os", kwlist, &pobj, &name))
		{	if (auto jour = static_cast<VI_TM_HJOUR>(PyLong_AsVoidPtr(pobj)); !PyErr_Occurred())
			{	if (auto meas = vi_tmRegistryGetMeas(jour, name))
				{	return PyLong_FromVoidPtr(meas);
				}
				PyErr_SetString(PyExc_RuntimeError, "Failed to get measurement");
			}
		}
		assert(false);
		return NULL;
	}

	PyObject* vi_timing_get_ticks(PyObject* Py_UNUSED(self), PyObject* noargs)
	{	assert(nullptr == noargs);
		VI_TM_TICK ticks = vi_tmGetTicks();
		return PyLong_FromLongLong(ticks);
	}

	PyObject* vi_timing_add_measurement(PyObject *Py_UNUSED(self), PyObject *args, PyObject *kwargs)
	{	static const char *kwlist[] = { "meas", "dur", "cnt", NULL };
		PyObject *pobj;
		Py_ssize_t dur_ss;
		Py_ssize_t cnt_ss;
		if (PyArg_ParseTupleAndKeywords(args, kwargs, "Onn", kwlist, &pobj, &dur_ss, &cnt_ss))
		{
			if (dur_ss < 0)
			{	PyErr_SetString(PyExc_ValueError, "dur must be non-negative");
		    }
			else if(cnt_ss < 0)
			{	PyErr_SetString(PyExc_ValueError, "cnt must be non-negative");
			}
			else if (auto meas = static_cast<VI_TM_HMEAS>(PyLong_AsVoidPtr(pobj)); !PyErr_Occurred())
			{	const auto dur = static_cast<VI_TM_TDIFF>(dur_ss);
				const auto cnt = static_cast<VI_TM_SIZE>(cnt_ss);
				vi_tmMeasurementAdd(meas, dur, cnt);
				Py_RETURN_NONE;
			}
		}
		assert(false);
		return NULL;
	}

	PyMethodDef vi_timing_methods[] =
	{
		{"dummy", vi_timing_dummy, METH_NOARGS, "Dummy function"},
		{"sleep", vi_timing_sleep, METH_O, "Sleep for a number of milliseconds"},
		{"version", vi_timing_version, METH_NOARGS, "Get the version string of the timing library" },
		{"init", reinterpret_cast<PyCFunction>(vi_timing_init), METH_VARARGS | METH_KEYWORDS, "Initialize the timing library"},
		{"shutdown", vi_timing_shutdown, METH_NOARGS, "Shutdown the timing library"},
		{"get_ticks", vi_timing_get_ticks, METH_NOARGS, "Get the current time in ticks"},
		{"registry_create", vi_timing_registry_create, METH_NOARGS, "Create a registry"},
		{"registry_close", vi_timing_registry_close, METH_O, "Close a registry"},
		{"create_measurement", reinterpret_cast<PyCFunction>(vi_timing_registry_get_meas), METH_VARARGS | METH_KEYWORDS, "Create a measurement"},
		{"add_measurement", reinterpret_cast<PyCFunction>(vi_timing_add_measurement), METH_VARARGS | METH_KEYWORDS, "Add a measurement"},
		{nullptr, nullptr, 0, nullptr}
	};

	PyModuleDef vi_timing_module =
	{	PyModuleDef_HEAD_INIT,
		"vi_timing",
		nullptr,
		-1,
		vi_timing_methods
	};
} // namespace

PyMODINIT_FUNC PyInit_vi_timing(void)
{
	return PyModule_Create(&vi_timing_module);
}
