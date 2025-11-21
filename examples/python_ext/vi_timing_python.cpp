// vi_timing_python.cpp
// Python extension module wrapping vi_timing library.
// Build: compile with Python.h and link against vi_timing library.
// Example: g++ -O2 -shared -fPIC vi_timing_py.cpp -o vi_timing.so \
//          -I/usr/include/python3.11 -lvi_timing

#include <vi_timing/vi_timing.h>
#include <Python.h>


namespace
{
	// Helpers: convert opaque handles to PyLong (uintptr_t) and back
	PyObject *py_from_reg(VI_TM_HREG h)
	{
		return PyLong_FromVoidPtr((void *)h);
	}
	VI_TM_HREG py_to_reg(PyObject *obj)
	{
		return (VI_TM_HREG)PyLong_AsVoidPtr(obj);
	}
	PyObject *py_from_meas(VI_TM_HMEAS h)
	{
		return PyLong_FromVoidPtr((void *)h);
	}
	VI_TM_HMEAS py_to_meas(PyObject *obj)
	{
		return (VI_TM_HMEAS)PyLong_AsVoidPtr(obj);
	}

	// ------------------- Wrappers -------------------

	PyObject *py_vi_tmGetTicks(PyObject *, PyObject *)
	{
		VI_TM_TICK t = vi_tmGetTicks();
		return PyLong_FromUnsignedLongLong(t);
	}

	PyObject *py_vi_tmGlobalInit(PyObject *, PyObject *args, PyObject *kwargs)
	{
		const char *kwlist[] = { "flags", "title", "footer", NULL };
		unsigned int flags = vi_tmReportDefault;
		const char *title = NULL;
		const char *footer = NULL;
		if (!PyArg_ParseTupleAndKeywords(args, kwargs, "|Iss", (char **)kwlist,
			&flags, &title, &footer))
			return NULL;
		VI_TM_RESULT r = vi_tmGlobalInit(flags, title, footer);
		return PyLong_FromLong(r);
	}

	PyObject *py_vi_tmRegistryCreate(PyObject *, PyObject *)
	{
		VI_TM_HREG h = vi_tmRegistryCreate();
		if (!h) Py_RETURN_NONE;
		return py_from_reg(h);
	}

	PyObject *py_vi_tmRegistryReset(PyObject *, PyObject *args)
	{
		PyObject *pobj;
		if (!PyArg_ParseTuple(args, "O", &pobj)) return NULL;
		vi_tmRegistryReset(py_to_reg(pobj));
		Py_RETURN_NONE;
	}

	PyObject *py_vi_tmRegistryClose(PyObject *, PyObject *args)
	{
		PyObject *pobj;
		if (!PyArg_ParseTuple(args, "O", &pobj)) return NULL;
		vi_tmRegistryClose(py_to_reg(pobj));
		Py_RETURN_NONE;
	}

	PyObject *py_vi_tmRegistryGetMeas(PyObject *, PyObject *args)
	{
		PyObject *pobj; const char *name;
		if (!PyArg_ParseTuple(args, "Os", &pobj, &name)) return NULL;
		VI_TM_HMEAS m = vi_tmRegistryGetMeas(py_to_reg(pobj), name);
		if (!m) Py_RETURN_NONE;
		return py_from_meas(m);
	}

	PyObject *py_vi_tmMeasurementAdd(PyObject *, PyObject *args)
	{
		PyObject *pobj; unsigned long long dur; Py_ssize_t cnt = 1;
		if (!PyArg_ParseTuple(args, "OK|n", &pobj, &dur, &cnt)) return NULL;
		vi_tmMeasurementAdd(py_to_meas(pobj), (VI_TM_TDIFF)dur, (VI_TM_SIZE)cnt);
		Py_RETURN_NONE;
	}

	PyObject *py_vi_tmMeasurementReset(PyObject *, PyObject *args)
	{
		PyObject *pobj;
		if (!PyArg_ParseTuple(args, "O", &pobj)) return NULL;
		vi_tmMeasurementReset(py_to_meas(pobj));
		Py_RETURN_NONE;
	}

	PyObject *py_vi_tmMeasurementGet(PyObject *, PyObject *args)
	{
		PyObject *pobj;
		if (!PyArg_ParseTuple(args, "O", &pobj)) return NULL;
		const char *name = nullptr;
		vi_tmStats_t stats{};
		vi_tmMeasurementGet(py_to_meas(pobj), &name, &stats);
		PyObject *dict = PyDict_New();
		PyDict_SetItemString(dict, "calls", PyLong_FromSize_t(stats.calls_));
#if VI_TM_STAT_USE_RAW
		PyDict_SetItemString(dict, "cnt", PyLong_FromSize_t(stats.cnt_));
		PyDict_SetItemString(dict, "sum", PyLong_FromUnsignedLongLong(stats.sum_));
#endif
#if VI_TM_STAT_USE_RMSE
		PyDict_SetItemString(dict, "flt_calls", PyLong_FromSize_t(stats.flt_calls_));
		PyDict_SetItemString(dict, "flt_cnt", PyFloat_FromDouble(stats.flt_cnt_));
		PyDict_SetItemString(dict, "flt_avg", PyFloat_FromDouble(stats.flt_avg_));
		PyDict_SetItemString(dict, "flt_ss", PyFloat_FromDouble(stats.flt_ss_));
#endif
#if VI_TM_STAT_USE_MINMAX
		PyDict_SetItemString(dict, "min", PyFloat_FromDouble(stats.min_));
		PyDict_SetItemString(dict, "max", PyFloat_FromDouble(stats.max_));
#endif
		return Py_BuildValue("sO", name ? name : "", dict);
	}

	PyObject *py_vi_tmStatsReset(PyObject *, PyObject *args)
	{
		PyObject *dict;
		if (!PyArg_ParseTuple(args, "O!", &PyDict_Type, &dict)) return NULL;
		vi_tmStats_t s{};
		vi_tmStatsReset(&s);
		PyDict_SetItemString(dict, "calls", PyLong_FromSize_t(s.calls_));
		Py_RETURN_NONE;
	}

	PyObject *py_vi_tmStatsIsValid(PyObject *, PyObject *args)
	{
		PyObject *dict;
		if (!PyArg_ParseTuple(args, "O!", &PyDict_Type, &dict)) return NULL;
		vi_tmStats_t s{};
		// minimal: calls
		PyObject *val = PyDict_GetItemString(dict, "calls");
		if (val) s.calls_ = (VI_TM_SIZE)PyLong_AsSize_t(val);
		VI_TM_RESULT r = vi_tmStatsIsValid(&s);
		return PyLong_FromLong(r);
	}

	PyObject *py_vi_tmStaticInfo(PyObject *, PyObject *args)
	{
		unsigned int info;
		if (!PyArg_ParseTuple(args, "I", &info)) return NULL;
		const void *p = vi_tmStaticInfo(info);
		if (!p) Py_RETURN_NONE;
		return PyLong_FromVoidPtr((void *)p);
	}

	PyObject *py_vi_tmRegistryReport(PyObject *, PyObject *args)
	{
		PyObject *pobj; unsigned int flags = vi_tmReportDefault;
		if (!PyArg_ParseTuple(args, "O|I", &pobj, &flags)) return NULL;
		VI_TM_RESULT r = vi_tmRegistryReport(py_to_reg(pobj), flags, vi_tmReportCb, nullptr);
		return PyLong_FromLong(r);
	}

	// ------------------- Method table -------------------

	PyMethodDef ViTimingMethods[] = {
		{ "GetTicks", (PyCFunction)py_vi_tmGetTicks, METH_NOARGS, "Get current tick count" },
		{ "GlobalInit", (PyCFunction)py_vi_tmGlobalInit, METH_VARARGS | METH_KEYWORDS, "Initialize global registry" },
		{ "RegistryCreate", (PyCFunction)py_vi_tmRegistryCreate, METH_NOARGS, "Create registry" },
		{ "RegistryReset", (PyCFunction)py_vi_tmRegistryReset, METH_VARARGS, "Reset registry" },
		{ "RegistryClose", (PyCFunction)py_vi_tmRegistryClose, METH_VARARGS, "Close registry" },
		{ "RegistryGetMeas", (PyCFunction)py_vi_tmRegistryGetMeas, METH_VARARGS, "Get measurement handle" },
		{ "MeasurementAdd", (PyCFunction)py_vi_tmMeasurementAdd, METH_VARARGS, "Add measurement data" },
		{ "MeasurementReset", (PyCFunction)py_vi_tmMeasurementReset, METH_VARARGS, "Reset measurement" },
		{ "MeasurementGet", (PyCFunction)py_vi_tmMeasurementGet, METH_VARARGS, "Get measurement info" },
		{ "StatsReset", (PyCFunction)py_vi_tmStatsReset, METH_VARARGS, "Reset stats dict" },
		{ "StatsIsValid", (PyCFunction)py_vi_tmStatsIsValid, METH_VARARGS, "Check stats validity" },
		{ "StaticInfo", (PyCFunction)py_vi_tmStaticInfo, METH_VARARGS, "Get static info" },
		{ "RegistryReport", (PyCFunction)py_vi_tmRegistryReport, METH_VARARGS, "Generate report" },
		{ NULL, NULL, 0, NULL }
	};

	// ------------------- Module definition -------------------

	struct PyModuleDef vi_timing_module = {
		PyModuleDef_HEAD_INIT,
		"vi_timing",   // name
		"Python binding for vi_timing library", // doc
		-1,
		ViTimingMethods
	};
}

PyMODINIT_FUNC PyInit_vi_timing(void) {
	PyObject* m = PyModule_Create(&vi_timing_module);
	if (!m) return NULL;

	// Add constants
	PyModule_AddIntConstant(m, "ReportDefault",        (int)vi_tmReportDefault);
	PyModule_AddIntConstant(m, "SortByTime",           (int)vi_tmSortByTime);
	PyModule_AddIntConstant(m, "SortByName",           (int)vi_tmSortByName);
	PyModule_AddIntConstant(m, "SortBySpeed",          (int)vi_tmSortBySpeed);
	PyModule_AddIntConstant(m, "SortByAmount",         (int)vi_tmSortByAmount);
	PyModule_AddIntConstant(m, "SortByMin",            (int)vi_tmSortByMin);
	PyModule_AddIntConstant(m, "SortByMax",            (int)vi_tmSortByMax);
	PyModule_AddIntConstant(m, "SortByCV",             (int)vi_tmSortByCV);
	PyModule_AddIntConstant(m, "SortMask",             (int)vi_tmSortMask);

	PyModule_AddIntConstant(m, "SortAscending",        (int)vi_tmSortAscending);

	PyModule_AddIntConstant(m, "ShowOverhead",         (int)vi_tmShowOverhead);
	PyModule_AddIntConstant(m, "ShowUnit",             (int)vi_tmShowUnit);
	PyModule_AddIntConstant(m, "ShowDuration",         (int)vi_tmShowDuration);
	PyModule_AddIntConstant(m, "ShowDurationEx",       (int)vi_tmShowDurationEx);
	PyModule_AddIntConstant(m, "ShowResolution",       (int)vi_tmShowResolution);
	PyModule_AddIntConstant(m, "ShowAux",              (int)vi_tmShowAux);
	PyModule_AddIntConstant(m, "ShowMask",             (int)vi_tmShowMask);

	PyModule_AddIntConstant(m, "HideHeader",           (int)vi_tmHideHeader);
	PyModule_AddIntConstant(m, "DoNotSubtractOverhead",(int)vi_tmDoNotSubtractOverhead);
	PyModule_AddIntConstant(m, "DoNotReport",          (int)vi_tmDoNotReport);
	PyModule_AddIntConstant(m, "ReportFlagsMask",      (int)vi_tmReportFlagsMask);

	// Статусные флаги сборки
	PyModule_AddIntConstant(m, "StatusDebug",          (int)vi_tmDebug);
	PyModule_AddIntConstant(m, "StatusShared",         (int)vi_tmShared);
	PyModule_AddIntConstant(m, "StatusThreadsafe",     (int)vi_tmThreadsafe);
	PyModule_AddIntConstant(m, "StatusStatUseBase",    (int)vi_tmStatUseBase);
	PyModule_AddIntConstant(m, "StatusStatUseRMSE",    (int)vi_tmStatUseRMSE);
	PyModule_AddIntConstant(m, "StatusStatUseFilter",  (int)vi_tmStatUseFilter);
	PyModule_AddIntConstant(m, "StatusStatUseMinMax",  (int)vi_tmStatUseMinMax);
	PyModule_AddIntConstant(m, "StatusMask",           (int)vi_tmStatusMask);

	// Маркеры успеха/ошибки
	PyModule_AddIntConstant(m, "SUCCESS",              (int)VI_SUCCESS);

	// Глобальный реестр как (void*)-1
	PyModule_AddObject(m, "HGLOBAL", PyLong_FromVoidPtr((void*)VI_TM_HGLOBAL));

	// Инфо-ключи (vi_tmInfo_e)
	PyModule_AddIntConstant(m, "InfoVer",              (int)vi_tmInfoVer);
	PyModule_AddIntConstant(m, "InfoVersion",          (int)vi_tmInfoVersion);
	PyModule_AddIntConstant(m, "InfoBuildNumber",      (int)vi_tmInfoBuildNumber);
	PyModule_AddIntConstant(m, "InfoResolution",       (int)vi_tmInfoResolution);
	PyModule_AddIntConstant(m, "InfoDuration",         (int)vi_tmInfoDuration);
	PyModule_AddIntConstant(m, "InfoDurationEx",       (int)vi_tmInfoDurationEx);
	PyModule_AddIntConstant(m, "InfoOverhead",         (int)vi_tmInfoOverhead);
	PyModule_AddIntConstant(m, "InfoSecPerUnit",       (int)vi_tmInfoSecPerUnit);
	PyModule_AddIntConstant(m, "InfoGitDescribe",      (int)vi_tmInfoGitDescribe);
	PyModule_AddIntConstant(m, "InfoGitCommit",        (int)vi_tmInfoGitCommit);
	PyModule_AddIntConstant(m, "InfoGitDateTime",      (int)vi_tmInfoGitDateTime);
	PyModule_AddIntConstant(m, "InfoFlags",            (int)vi_tmInfoFlags);
	PyModule_AddIntConstant(m, "InfoCount",            (int)vi_tmInfoCount_);

	return m;
}
