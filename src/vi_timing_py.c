#define PY_SSIZE_T_CLEAN

#include <Python.h>
#include <vi_timing/vi_timing.h>

typedef struct {
	PyObject_HEAD
	VI_TM_HMEAS handle;
	VI_TM_TICK start;
} PyMeasurement;

static PyObject* PyMeasurement_new(PyTypeObject* type, PyObject* args, PyObject* kwds)
{	(void)kwds;
	(void)args;
	PyMeasurement* self = (PyMeasurement*)type->tp_alloc(type, 0);
	self->handle = NULL;
	self->start = 0;
	return (PyObject*)self;
}

static int PyMeasurement_init(PyMeasurement* self, PyObject* args, PyObject* kwds)
{	(void)kwds;
	const char* name;
	if (!PyArg_ParseTuple(args, "s", &name))
		return -1;
	self->handle = vi_tmRegistryGetMeas(VI_TM_HGLOBAL, name);
	if (!self->handle) {
		PyErr_SetString(PyExc_RuntimeError, "Failed to get measurement handle");
		return -1;
	}
	return 0;
}

static PyObject* PyMeasurement_add(PyMeasurement* self, PyObject* args) {
	uint64_t duration;
	size_t count = 1;
	if (!PyArg_ParseTuple(args, "K|n", &duration, &count))
		return NULL;
	vi_tmMeasurementAdd(self->handle, duration, count);
	Py_RETURN_NONE;
}

static PyObject* PyMeasurement_enter(PyMeasurement* self, PyObject* Py_UNUSED(ignored))
{	(void)Py_UNUSED(ignored);
	self->start = vi_tmGetTicks();
	Py_INCREF(self);
	return (PyObject*)self;
}

static PyObject* PyMeasurement_exit(PyMeasurement* self, PyObject* args)
{	(void)args;
	VI_TM_TICK end = vi_tmGetTicks();
	vi_tmMeasurementAdd(self->handle, end - self->start, 1);
	Py_RETURN_NONE;
}

static PyMethodDef PyMeasurement_methods[] = {
	{"add", (PyCFunction)PyMeasurement_add, METH_VARARGS, "Add a duration to the measurement"},
	{"__enter__", (PyCFunction)PyMeasurement_enter, METH_NOARGS, "Start timing"},
	{"__exit__", (PyCFunction)PyMeasurement_exit, METH_VARARGS, "Stop timing and record"},
	{NULL}
};

static PyTypeObject PyMeasurementType = {
	PyVarObject_HEAD_INIT(NULL, 0)
	.tp_name = "vi_timing.Measurement",
	.tp_basicsize = sizeof(PyMeasurement),
	.tp_flags = Py_TPFLAGS_DEFAULT,
	.tp_new = PyMeasurement_new,
	.tp_init = (initproc)PyMeasurement_init,
	.tp_methods = PyMeasurement_methods,
};

static PyObject* vi_tm_get_ticks(PyObject* self, PyObject* Py_UNUSED(ignored))
{	(void)self;
	(void)Py_UNUSED(ignored);
	return PyLong_FromUnsignedLongLong(vi_tmGetTicks());
}

static PyObject* vi_tm_init(PyObject* self, PyObject* Py_UNUSED(ignored))
{	(void)self;
	(void)Py_UNUSED(ignored);
	int res = vi_tmInit("Timing report:\n", vi_tmReportDefault, 0);
	return PyLong_FromLong(res);
}

static PyObject* vi_tm_shutdown(PyObject* self, PyObject* Py_UNUSED(ignored))
{	(void)self;
	(void)Py_UNUSED(ignored);
	vi_tmShutdown();
	Py_RETURN_NONE;
}

static VI_TM_RESULT VI_SYS_CALL report_callback(const char* str, void* ctx)
{	(void)ctx;
	PySys_WriteStdout("%s", str);
	return 0;
}

static PyObject* vi_tm_report(PyObject* self, PyObject* Py_UNUSED(ignored))
{	(void)self;
	(void)Py_UNUSED(ignored);
	vi_tmReport(VI_TM_HGLOBAL, vi_tmReportDefault, report_callback, NULL);
	Py_RETURN_NONE;
}

static PyMethodDef module_methods[] = {
	{"init", vi_tm_init, METH_NOARGS, "Initialize vi_timing"},
	{"shutdown", vi_tm_shutdown, METH_NOARGS, "Shutdown vi_timing"},
	{"get_ticks", vi_tm_get_ticks, METH_NOARGS, "Get current tick count"},
	{"report", vi_tm_report, METH_NOARGS, "Print timing report"},
	{NULL}
};

static struct PyModuleDef vi_timing_module = {
	PyModuleDef_HEAD_INIT,
	"vi_timing",
	"Native wrapper for vi_timing library",
	-1,
	module_methods
};

PyMODINIT_FUNC PyInit_vi_timing(void) {
	//if (PyType_Ready(&PyMeasurementType) < 0)
	//	return NULL;

	PyObject* m = PyModule_Create(&vi_timing_module);
	if (!m)
		return NULL;

	//Py_INCREF(&PyMeasurementType);
	//PyModule_AddObject(m, "Measurement", (PyObject*)&PyMeasurementType);
	return m;
}
