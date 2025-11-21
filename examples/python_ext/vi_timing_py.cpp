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

extern "C" API_EXPORT void tmDummy() noexcept
{	/**/
}

namespace
{
	PyObject* py_tmDummy(PyObject* Py_UNUSED(self), PyObject* noargs)
	{	assert(nullptr == noargs);
		tmDummy();
		Py_RETURN_NONE;
	}

	PyObject* py_vi_tmGetTicks(PyObject* Py_UNUSED(self), PyObject* noargs)
	{	assert(nullptr == noargs);
		VI_TM_TICK ticks = vi_tmGetTicks();
		return PyLong_FromLongLong(ticks);
	}

	PyObject* py_vi_tmGlobalInit(PyObject *Py_UNUSED(self), PyObject *args, PyObject *kwargs)
	{	static constexpr const char * kwlist[] = { "flags", "title", "footer", nullptr };
		unsigned long long flags = vi_tmReportDefault;
		const char *title = nullptr;
		const char *footer = nullptr;
		if (PyArg_ParseTupleAndKeywords(args, kwargs, "K|ss", const_cast<char**>(kwlist), &flags, &title, &footer))
		{	if (flags > std::numeric_limits<uint32_t>::max())
			{	PyErr_SetString(PyExc_ValueError, "flags out of range for uint32_t");
			}
			else if (long result = vi_tmGlobalInit(static_cast<uint32_t>(flags), title, footer); VI_FAILED(result))
			{	PyErr_SetString(PyExc_RuntimeError, "Failed to initialize global timing report");
			}
			else
			{	return PyLong_FromLong(result);
			}
		}
		assert(false);
		return NULL;
	}

	PyObject* py_vi_tmRegistryCreate(PyObject* Py_UNUSED(self), PyObject* noargs)
	{	assert(nullptr == noargs);
		if (VI_TM_HREG jour = vi_tmRegistryCreate())
		{	return PyLong_FromVoidPtr(jour);
		}
		PyErr_SetString(PyExc_RuntimeError, "Failed to create registry");
		assert(false);
		return NULL;
	}

	PyObject* py_vi_tmRegistryClose(PyObject* Py_UNUSED(self), PyObject* arg)
	{	if (auto jour = static_cast<VI_TM_HREG>(PyLong_AsVoidPtr(arg)); !PyErr_Occurred())
		{	vi_tmRegistryClose(jour);
			Py_RETURN_NONE;
		}
		assert(false);
		return NULL;
	}

	PyObject* py_vi_tmRegistryGetMeas(PyObject* Py_UNUSED(self), PyObject* args, PyObject* kwargs)
	{	static constexpr const char* kwlist[] = {"jour", "name", NULL};
		PyObject* pobj;
		const char* name;
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

	PyObject* py_vi_tmMeasurementAdd(PyObject *Py_UNUSED(self), PyObject *args, PyObject *kwargs)
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

	PyObject *py_vi_tmRegistryReport(PyObject *Py_UNUSED(self), PyObject *args, PyObject *kwargs)
	{	static constexpr const char *kwlist[] = { "jour", "flags", "cb", "ctx", NULL };
		PyObject *p_jour, *p_cb = Py_None, *p_ctx = Py_None;
		int flags = vi_tmReportDefault;
		if (PyArg_ParseTupleAndKeywords(args, kwargs, "O|iOO", const_cast<char**>(kwlist), &p_jour, &flags, &p_cb, &p_ctx))
		{	if (auto jour = static_cast<VI_TM_HREG>(PyLong_AsVoidPtr(p_jour)); !PyErr_Occurred())
			{	vi_tmReportCb_t cb = vi_tmReportCb;
				void *ctx = nullptr;
				// TODO: p_cb->cb; p_ctx->ctx;
				const auto result = vi_tmRegistryReport(jour, static_cast<VI_TM_FLAGS>(flags), cb, ctx);
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
		{"dummy", py_tmDummy, METH_NOARGS, "Dummy function"},
		{"GlobalInit", reinterpret_cast<PyCFunction>(py_vi_tmGlobalInit), METH_VARARGS | METH_KEYWORDS, "Initialize the timing library"},
		{"GetTicks", py_vi_tmGetTicks, METH_NOARGS, "Get the current time in ticks"},
		{"RegistryCreate", py_vi_tmRegistryCreate, METH_NOARGS, "Create a registry"},
		{"RegistryClose", py_vi_tmRegistryClose, METH_O, "Close a registry"},
		{"RegistryGetMeas", reinterpret_cast<PyCFunction>(py_vi_tmRegistryGetMeas), METH_VARARGS | METH_KEYWORDS, "Create a measurement"},
		{"MeasurementAdd", reinterpret_cast<PyCFunction>(py_vi_tmMeasurementAdd), METH_VARARGS | METH_KEYWORDS, "Add a measurement"},
		{"RegistryReport", reinterpret_cast<PyCFunction>(py_vi_tmRegistryReport), METH_VARARGS | METH_KEYWORDS, "Generate a report for a registry"},
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
	auto m = PyModule_Create(&vi_timing_module);
	if (!m)
	{	return NULL;
	}

	PyModule_AddIntConstant(m, "ReportDefault", (int)vi_tmReportDefault);
	PyModule_AddIntConstant(m, "SUCCESS", (int)VI_SUCCESS);
	PyModule_AddObject(m, "HGLOBAL", PyLong_FromVoidPtr((void *)VI_TM_HGLOBAL));

	return m;
}
