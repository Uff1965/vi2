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
	void vi_Dummy() noexcept
	{	/**/
	}
} // extern "C"

namespace
{
	PyObject* vi_timing_dummy(PyObject* Py_UNUSED(self), PyObject* noargs)
	{	assert(nullptr == noargs);
		::vi_Dummy();
		Py_RETURN_NONE;
	}

	PyObject* vi_timing_init(PyObject *Py_UNUSED(self), PyObject *args, PyObject *kwargs)
	{	static constexpr const char * kwlist[] = { "title", "report_flags", "flags", nullptr };
		const char *title;
		int report_flags;
		int flags;
		if (PyArg_ParseTupleAndKeywords(args, kwargs, "sii", const_cast<char**>(kwlist), &title, &report_flags, &flags))
		{	if (long result = vi_tmInit(title, report_flags, flags); VI_SUCCEEDED(result))
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
		if (VI_TM_HREG jour = vi_tmRegistryCreate())
		{	return PyLong_FromVoidPtr(jour);
		}
		PyErr_SetString(PyExc_RuntimeError, "Failed to create registry");
		assert(false);
		return NULL;
	}

	PyObject* vi_timing_registry_close(PyObject* Py_UNUSED(self), PyObject* arg)
	{	if (auto jour = static_cast<VI_TM_HREG>(PyLong_AsVoidPtr(arg)); !PyErr_Occurred())
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
		if (PyArg_ParseTupleAndKeywords(args, kwargs, "Os", const_cast<char**>(kwlist), &pobj, &name))
		{	auto jour = static_cast<VI_TM_HREG>(PyLong_AsVoidPtr(pobj));
			if ( !PyErr_Occurred())
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
	{	static constexpr const char *kwlist[] = { "meas", "dur", "cnt", NULL };
		PyObject *pobj;
		Py_ssize_t dur_ss;
		Py_ssize_t cnt_ss = 1;
		if (PyArg_ParseTupleAndKeywords(args, kwargs, "On|n", const_cast<char**>(kwlist), &pobj, &dur_ss, &cnt_ss))
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

	PyObject *vi_timing_report(PyObject *Py_UNUSED(self), PyObject *args, PyObject *kwargs)
	{	static constexpr const char *kwlist[] = { "jour", "flags", "cb", "ctx", NULL };
		PyObject *p_jour, *p_cb = Py_None, *p_ctx = Py_None;
		int flags = vi_tmReportDefault;
		if (PyArg_ParseTupleAndKeywords(args, kwargs, "O|iOO", const_cast<char**>(kwlist), &p_jour, &flags, &p_cb, &p_ctx))
		{	if (auto jour = static_cast<VI_TM_HREG>(PyLong_AsVoidPtr(p_jour)); !PyErr_Occurred())
			{	vi_tmReportCb_t cb = vi_tmReportCb;
				void *ctx = nullptr;
				// TODO: p_cb->cb; p_ctx->ctx;
				const auto result = vi_tmReport(jour, static_cast<VI_TM_FLAGS>(flags), cb, ctx);
				if (VI_SUCCEEDED(result))
				{	return PyLong_FromLongLong(result);
				}
				PyErr_SetString(PyExc_RuntimeError, "Failed to generate report");
			}
		}
		assert(false);
		return NULL;
	}

	PyMethodDef vi_timing_methods[] =
	{
		{"dummy", vi_timing_dummy, METH_NOARGS, "Dummy function"},
		{"init", reinterpret_cast<PyCFunction>(vi_timing_init), METH_VARARGS | METH_KEYWORDS, "Initialize the timing library"},
		{"shutdown", vi_timing_shutdown, METH_NOARGS, "Shutdown the timing library"},
		{"get_ticks", vi_timing_get_ticks, METH_NOARGS, "Get the current time in ticks"},
		{"registry_create", vi_timing_registry_create, METH_NOARGS, "Create a registry"},
		{"registry_close", vi_timing_registry_close, METH_O, "Close a registry"},
		{"create_measurement", reinterpret_cast<PyCFunction>(vi_timing_registry_get_meas), METH_VARARGS | METH_KEYWORDS, "Create a measurement"},
		{"add_measurement", reinterpret_cast<PyCFunction>(vi_timing_add_measurement), METH_VARARGS | METH_KEYWORDS, "Add a measurement"},
		{"report", reinterpret_cast<PyCFunction>(vi_timing_report), METH_VARARGS | METH_KEYWORDS, "Generate a report for a registry"},
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
