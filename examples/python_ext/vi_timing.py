# vi_timing_mod.py

import ctypes # For loading the C-extension and calling its functions
import importlib # For dynamic loading of the C-extension module
import importlib.machinery # For getting the extension suffixes
import importlib.util # For loading the module from a file
import os # For file path manipulations
import sys # For accessing sys.path
import time # For performance measurements
import timeit # For timing code execution
from contextlib import contextmanager # For creating context managers
from ctypes import (string_at, c_void_p, c_char_p, c_int32, c_uint32, c_uint64, c_size_t, c_double) # For defining C types

###############################################################################
# Helper to find the native-extension file for a module
def find_extension_file(name, first_dir = None)->(str|None):
	"""
	Searches for the native-extension file for the 'name' module.
	First looks in first_dir (if specified), then in all paths in sys.path.
	Standart suffixes from importlib.machinery.EXTENSION_SUFFIXES are used.
	Debug builds of Python add the suffix '_d'!!!
	Returns the absolute path to the found file or None.
	"""
	search_paths = []
	if first_dir is not None:
		search_paths.append(first_dir)
	search_paths.extend(sys.path)

	for base in search_paths:
		for suffix in importlib.machinery.EXTENSION_SUFFIXES: # The debug version of Python adds the suffix _d.
			candidate_path = os.path.join(base, name + suffix)
			if os.path.isfile(candidate_path):
				return os.path.relpath(candidate_path)
	return None

def load_module(path: str)->None:
	spec = importlib.util.spec_from_file_location("vi_timing", path)
	module = importlib.util.module_from_spec(spec)
	sys.modules["vi_timing_mod"] = module
	return None

module_path = find_extension_file("vi_timing_py", os.getcwd())
if(not module_path):
	raise ModuleNotFoundError("Cannot find the 'vi_timing_py' module.")

print(f"A file with the matching name '{module_path}' was found.")

print("\tNative-extension module 'vi_timing' importing...", end=" ")
load_module(module_path)
import vi_timing_mod
print("done.")

print(f"\tNative shared library '{module_path}' loading...", end=" ")
vi_timing_lib = ctypes.CDLL(module_path)
print("done.")

###############################################################################
VI_TM_RESULT = c_int32
VI_TM_FLAGS = c_uint32
VI_TM_SIZE = c_size_t
VI_TM_TICK = c_uint64
VI_TM_TDIFF = VI_TM_TICK
VI_TM_HMEAS = c_void_p
VI_TM_HJOUR = c_void_p
VI_TM_HGLOBAL = VI_TM_HJOUR(-1).value

vi_timing_lib.DummyVoidC.argtypes = []
vi_timing_lib.DummyVoidC.restype = None

vi_timing_lib.DummyFloatC.argtypes = [c_double]
vi_timing_lib.DummyFloatC.restype = c_double

vi_timing_lib.vi_tmStaticInfo.argtypes = [VI_TM_FLAGS]
vi_timing_lib.vi_tmStaticInfo.restype = c_void_p

vi_timing_lib.vi_tmGetTicks.argtypes = []
vi_timing_lib.vi_tmGetTicks.restype = VI_TM_TICK

vi_timing_lib.vi_tmRegistryCreate.argtypes = []
vi_timing_lib.vi_tmRegistryCreate.restype = VI_TM_HJOUR

vi_timing_lib.vi_tmRegistryClose.argtypes = [VI_TM_HJOUR]
vi_timing_lib.vi_tmRegistryClose.restype = None

vi_timing_lib.vi_tmMeasurementAdd.argtypes = [VI_TM_HMEAS, VI_TM_TDIFF, VI_TM_SIZE]
vi_timing_lib.vi_tmMeasurementAdd.restype = None

vi_timing_lib.vi_tmRegistryGetMeas.argtypes = [VI_TM_HJOUR, c_char_p]
vi_timing_lib.vi_tmRegistryGetMeas.restype = VI_TM_HMEAS

vi_timing_lib.vi_tmRegistryReport.argtypes = [VI_TM_HJOUR, VI_TM_FLAGS, c_void_p, c_void_p]
vi_timing_lib.vi_tmRegistryReport.restype = VI_TM_RESULT

def DummyVoidPy()->None:
	return None

def DummyFloatPy(f: float)->float:
	return f

def check_lib():
	@contextmanager
	def reg_lib():
		reg = vi_timing_lib.vi_tmRegistryCreate()
		yield reg
		callback_addr = ctypes.cast(vi_timing_lib.vi_tmReportCb, ctypes.c_void_p).value
		vi_timing_lib.vi_tmRegistryReport(reg, 0, callback_addr, None)
		vi_timing_lib.vi_tmRegistryClose(reg)

	print("\nTimes when using exported functions via ctypes:")
	with reg_lib() as registry:
		tick = vi_timing_lib.vi_tmGetTicks

		hmes = vi_timing_lib.vi_tmRegistryGetMeas(registry, "Empty".encode("utf-8"))
		def measure():
			s = tick(); f = tick()
			vi_timing_lib.vi_tmMeasurementAdd(hmes, f - s, 1)
		print(f"\tmeasure Empty:\t{timeit.timeit(measure, number=100_000)*10:.2g} us")

		hmes = vi_timing_lib.vi_tmRegistryGetMeas(registry, "DummyVoidPy".encode("utf-8"))
		def measure():
			s = tick()
			DummyVoidPy()
			f = tick()
			vi_timing_lib.vi_tmMeasurementAdd(hmes, f - s, 1)
		print(f"\tmeasure DummyVoidPy:\t{timeit.timeit(measure, number=100_000)*10:.2g} us")

		hmes = vi_timing_lib.vi_tmRegistryGetMeas(registry, "vi_timing_lib.DummyVoidC".encode("utf-8"))
		def measure():
			s = tick()
			vi_timing_lib.DummyVoidC();
			f = tick()
			vi_timing_lib.vi_tmMeasurementAdd(hmes, f - s, 1)
		print(f"\tmeasure vi_timing_lib.DummyVoidC:\t{timeit.timeit(measure, number=100_000)*10:.2g} us")

def check_mod():
	@contextmanager
	def reg_mod():
		reg_mod = vi_timing_mod.RegistryCreate()
		yield reg_mod
		vi_timing_mod.RegistryReport(reg_mod, 0)
		vi_timing_mod.RegistryClose(reg_mod)

	print("\nTimes when using the C-extension:")
	with reg_mod() as reg_mod:
		tick = vi_timing_mod.GetTicks

		hmes = vi_timing_mod.RegistryGetMeas(reg_mod, "Empty")
		def measure():
			s = tick(); f = tick()
			vi_timing_mod.MeasurementAdd(hmes, f - s)
		print(f"\tmeasure Empty:\t{timeit.timeit(measure, number=100_000)*10:.2g} us")

		hmes = vi_timing_mod.RegistryGetMeas(reg_mod, "DummyVoidPy")
		def measure():
			s = tick()
			DummyVoidPy()
			f = tick()
			vi_timing_mod.MeasurementAdd(hmes, f - s)
		print(f"\tmeasure DummyVoidPy:\t{timeit.timeit(measure, number=100_000)*10:.2g} us")

		hmes = vi_timing_mod.RegistryGetMeas(reg_mod, "vi_timing_mod.DummyVoidC")
		def measure():
			s = tick()
			vi_timing_mod.DummyVoidC()
			f = tick()
			vi_timing_mod.MeasurementAdd(hmes, f - s)
		print(f"\tmeasure vi_timing_mod.DummyVoidC:\t{timeit.timeit(measure, number=100_000)*10:.2g} us")

def float_func_metrics(func, val: float):
	print(f"\t{func.__module__}.{func.__name__}: \t{timeit.timeit(
		stmt='func(val)',
		globals={'val': val, 'func': func},
		number=1_000_000
	):.2g} us")

def void_func_metrics(func):
	print(f"\t{func.__module__}.{func.__name__}: \t{timeit.timeit(func, number=1_000_000):.2g} us")

def func_metrics(func, unit: float):
	total = 0.0
	for _ in range(func_metrics.cnt):
		total -= func() - func()
	duration = timeit.timeit(func, number=1_000_000)
	print(f"{func.__module__}.{func.__name__}: \t{duration:.2g} us \t[{ total * unit * 1e6 / func_metrics.cnt:.2g} us]")
func_metrics.cnt = 1_000_000

###############################################################################
# Provide a simple CLI demo entrypoint
if __name__ == "__main__":
	print(f"vi_timing: {string_at(vi_timing_lib.vi_tmStaticInfo(1)).decode('utf-8')}\n")

	global_s_mod = vi_timing_mod.GetTicks()
	global_s_lib = vi_timing_lib.vi_tmGetTicks()

	# vi_tmInfoSecPerUnit
	seconds_per_unit = ctypes.cast(vi_timing_lib.vi_tmStaticInfo(7), ctypes.POINTER(ctypes.c_double)).contents.value

	print("Time measurement function metrics:\nFunction name \t\tDuration \t[Resolution]")
	func_metrics(time.perf_counter, 1)
	func_metrics(vi_timing_mod.GetTicks, seconds_per_unit)
	func_metrics(vi_timing_lib.vi_tmGetTicks, seconds_per_unit)
	print("")

	print("Function call duration measured by timeit:")
	void_func_metrics(DummyVoidPy)
	void_func_metrics(vi_timing_mod.DummyVoidC)
	void_func_metrics(vi_timing_lib.DummyVoidC)
	val = 3.14
	float_func_metrics(DummyFloatPy, val)
	float_func_metrics(vi_timing_mod.DummyFloatC, val)
	float_func_metrics(vi_timing_lib.DummyFloatC, val)

	check_mod()
	check_lib()

	print("")
	print("Finish timing tests...")
	global_f_mod = vi_timing_mod.GetTicks()
	global_f_lib = vi_timing_lib.vi_tmGetTicks()

	global_mod = vi_timing_mod.RegistryGetMeas(vi_timing_mod.HGLOBAL, "global_mod")
	vi_timing_mod.MeasurementAdd(global_mod, global_f_mod - global_s_mod)

	global_lib = vi_timing_lib.vi_tmRegistryGetMeas(VI_TM_HGLOBAL, "global_lib".encode("utf-8"))
	vi_timing_lib.vi_tmMeasurementAdd(global_lib, global_f_lib - global_s_lib, 1)

	print("")

	if sys.stdin.isatty() and os.getenv("VI_TM_NO_PAUSE") != "1":
		input("Press Enter, for close...")
