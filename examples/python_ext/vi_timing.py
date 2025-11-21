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
from ctypes import (string_at, c_void_p, c_char_p, c_int32, c_uint32, c_uint64, c_size_t) # For defining C types
from typing import Callable

###############################################################################
# Helper to find the native-extension file for a module
def find_extension_file(name, first_dir = None):
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
		for suf in importlib.machinery.EXTENSION_SUFFIXES: # The debug version of Python adds the suffix _d.
			candidate_path = os.path.join(base, name + suf)
			if os.path.isfile(candidate_path):
				return os.path.abspath(candidate_path)
	return None
module_path = find_extension_file("vi_timing_py", os.getcwd())
print(f"Loading vi_timing module from: '{module_path}'")

def load_module(path: str):
	spec = importlib.util.spec_from_file_location("vi_timing", path)
	module = importlib.util.module_from_spec(spec)
	sys.modules["vi_timing_mod"] = module
	return None
load_module(module_path)
import vi_timing_mod

print(f"Load library '{module_path}' by ctype\n")
vi_timing_lib = ctypes.CDLL(module_path)

###############################################################################
VI_TM_RESULT = c_int32
VI_TM_FLAGS = c_uint32
VI_TM_SIZE = c_size_t
VI_TM_TICK = c_uint64
VI_TM_TDIFF = VI_TM_TICK
VI_TM_HMEAS = c_void_p
VI_TM_HJOUR = c_void_p
VI_TM_HGLOBAL = VI_TM_HJOUR(-1).value

vi_timing_lib.tmDummy.argtypes = []
vi_timing_lib.tmDummy.restype = None

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

vi_timing_lib.vi_tmRegistryGetMeas.restype = VI_TM_HMEAS
vi_timing_lib.vi_tmRegistryGetMeas.argtypes = [VI_TM_HJOUR, c_char_p]

vi_timing_lib.vi_tmRegistryReport.restype = VI_TM_RESULT
vi_timing_lib.vi_tmRegistryReport.argtypes = [VI_TM_HJOUR, VI_TM_FLAGS, c_void_p, c_void_p]

perf = time.perf_counter
mod_tick = vi_timing_mod.GetTicks
lib_tick = vi_timing_lib.vi_tmGetTicks

def check_lib():
	@contextmanager
	def reg_lib():
		reg_lib = vi_timing_lib.vi_tmRegistryCreate()
		yield reg_lib
		callback_addr = ctypes.cast(vi_timing_lib.vi_tmReportCb, ctypes.c_void_p).value
		vi_timing_lib.vi_tmRegistryReport(reg_lib, 0, callback_addr, None)
		vi_timing_lib.vi_tmRegistryClose(reg_lib)

	print("\nTimes when using exported functions via ctypes:")
	print(f"\ttimeit.timeit(vi_timing_lib.tmDummy): \t{timeit.timeit(vi_timing_lib.tmDummy, number=1_000)*1_000:.2g} us")
	with reg_lib() as reg_lib:
		empty = vi_timing_lib.vi_tmRegistryGetMeas(reg_lib, "empty_fn".encode("utf-8"))
		def measure():
			s = lib_tick()
			empty_fn()
			f = lib_tick()
			vi_timing_lib.vi_tmMeasurementAdd(empty, f - s, 1)
		print(f"\tmeasure empty:\t{timeit.timeit(measure, number=100_000)*10:.2g} us")
		dummy = vi_timing_lib.vi_tmRegistryGetMeas(reg_lib, "dummy".encode("utf-8"))
		def measure():
			s = lib_tick()
			vi_timing_lib.tmDummy();
			f = lib_tick()
			vi_timing_lib.vi_tmMeasurementAdd(dummy, f - s, 1)
		print(f"\tmeasure dummy:\t{timeit.timeit(measure, number=100_000)*10:.2g} us")

def check_mod():
	@contextmanager
	def reg_mod():
		reg_mod = vi_timing_mod.RegistryCreate()
		yield reg_mod
		vi_timing_mod.RegistryReport(reg_mod, 0)
		vi_timing_mod.RegistryClose(reg_mod)

	print("\nTimes when using the C-extension:")
	print(f"\ttimeit.timeit(vi_timing_mod.dummy): \t{timeit.timeit(vi_timing_mod.dummy, number=1_000)*1_000:.2g} us")
	with reg_mod() as reg_mod:
		empty = vi_timing_mod.RegistryGetMeas(reg_mod, "empty_fn")
		def measure():
			s = mod_tick()
			empty_fn()
			f = mod_tick()
			vi_timing_mod.MeasurementAdd(empty, f - s)
		print(f"\tmeasure empty:\t{timeit.timeit(measure, number=100_000)*10:.2g} us")
		dummy = vi_timing_mod.RegistryGetMeas(reg_mod, "dummy")
		def measure():
			s = mod_tick()
			vi_timing_mod.dummy()
			f = mod_tick()
			vi_timing_mod.MeasurementAdd(dummy, f - s)
		print(f"\tmeasure dummy:\t{timeit.timeit(measure, number=100_000)*10:.2g} us")

###############################################################################
# Provide a simple CLI demo entrypoint
if __name__ == "__main__":
	print(f"vi_timing_lib::vi_tmStaticInfo: \t{string_at(vi_timing_lib.vi_tmStaticInfo(c_uint32(1))).decode('utf-8')}\n")

	print("Start timing tests...")
	global_s_mod = mod_tick()
	global_s_lib = lib_tick()

	def empty_fn():
		pass

	print(f"\ntimeit.timeit(empty_fn()): \t{timeit.timeit(empty_fn, number=1_000)*1_000:.2g} us")

	check_mod()
	check_lib()

	print("")
	print("Finish timing tests...")
	global_f_mod = mod_tick()
	global_f_lib = lib_tick()

	global_mod = vi_timing_mod.RegistryGetMeas(VI_TM_HGLOBAL, "global_mod")
	vi_timing_mod.MeasurementAdd(global_mod, global_f_mod - global_s_mod)

	global_lib = vi_timing_lib.vi_tmRegistryGetMeas(VI_TM_HGLOBAL, "global_lib".encode("utf-8"))
	vi_timing_lib.vi_tmMeasurementAdd(global_lib, global_f_lib - global_s_lib, 1)

	print("")

	double_ptr = ctypes.cast(vi_timing_lib.vi_tmStaticInfo(7), ctypes.POINTER(ctypes.c_double)) # vi_tmInfoSecPerUnit
	useconds_per_unit = double_ptr.contents.value

	def func_metrics(func: Callable[[], float], unit: float):
		total = 0.0
		for _ in range(func_metrics.cnt):
			total -= func() - func()
		print(f"{func.__name__}:\t{timeit.timeit(func, number=1_000_000):.2g} us [{ total * unit * 1e6 / func_metrics.cnt:.2g} us]")
	func_metrics.cnt = 1_000_000

	func_metrics(perf, 1)
	func_metrics(mod_tick, useconds_per_unit)
	func_metrics(lib_tick, useconds_per_unit)
	print("")

	if sys.stdin.isatty() and os.getenv("VI_TM_NO_PAUSE") != "1":
		input("Press Enter, for close...")
