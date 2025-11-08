# vi_timing.py

import ctypes
import importlib
import importlib.machinery
import importlib.util
import os
import sys
import timeit
from contextlib import contextmanager
from ctypes import (
	string_at, c_void_p, c_char_p, c_int32, c_uint32, c_uint64, c_size_t, 
#    Structure, c_double,
#    c_int32, POINTER, CFUNCTYPE, byref, create_string_buffer
)

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

def load_module(path: str):
	spec = importlib.util.spec_from_file_location("vi_timing", path)
	module = importlib.util.module_from_spec(spec)
	sys.modules["vi_timing"] = module
	return None

module_path = find_extension_file("vi_timing_py", os.getcwd())
print(f"Loading vi_timing module from: {module_path}\n")

load_module(module_path)
import vi_timing
lib = ctypes.CDLL(module_path)

###############################################################################
VI_TM_RESULT = c_int32
VI_TM_FLAGS = c_uint32
VI_TM_SIZE = c_size_t
VI_TM_TICK = c_uint64
VI_TM_TDIFF = VI_TM_TICK
VI_TM_HMEAS = c_void_p
VI_TM_HJOUR = c_void_p
#VI_TM_HGLOBAL = ctypes.cast(VI_TM_HJOUR(-1 & 0xFFFFFFFFFFFFFFFF), VI_TM_HJOUR)
VI_TM_HGLOBAL = c_void_p(-1 & 0xFFFFFFFFFFFFFFFF).value

lib.vi_Dummy.argtypes = []
lib.vi_Dummy.restype = None

lib.vi_Sleep.argtypes = [c_uint32]
lib.vi_Sleep.restype = None

lib.vi_tmStaticInfo.argtypes = [VI_TM_FLAGS]
lib.vi_tmStaticInfo.restype = c_void_p

lib.vi_tmGetTicks.argtypes = []
lib.vi_tmGetTicks.restype = VI_TM_TICK

lib.vi_tmRegistryCreate.argtypes = []
lib.vi_tmRegistryCreate.restype = VI_TM_HJOUR

lib.vi_tmRegistryClose.argtypes = [VI_TM_HJOUR]
lib.vi_tmRegistryClose.restype = None

lib.vi_tmMeasurementAdd.argtypes = [VI_TM_HMEAS, VI_TM_TDIFF, VI_TM_SIZE]
lib.vi_tmMeasurementAdd.restype = None

lib.vi_tmRegistryGetMeas.restype = VI_TM_HMEAS
lib.vi_tmRegistryGetMeas.argtypes = [VI_TM_HJOUR, c_char_p]

lib.vi_tmReport.restype = VI_TM_RESULT
lib.vi_tmReport.argtypes = [VI_TM_HJOUR, VI_TM_FLAGS, c_void_p, c_void_p]

###############################################################################
# Provide a simple CLI demo entrypoint
if __name__ == "__main__":
	print(f"vi_timing.version: \t{vi_timing.version()}")
	print(f"lib::vi_tmStaticInfo: \t{string_at(lib.vi_tmStaticInfo(c_uint32(2))).decode("utf-8")}")

	global_mod = vi_timing.create_measurement(VI_TM_HGLOBAL, "global_mod")
	global_lib = lib.vi_tmRegistryGetMeas(VI_TM_HGLOBAL, "global_lib".encode("utf-8"))

	global_s_mod = vi_timing.get_ticks()
	global_s_lib = lib.vi_tmGetTicks()

	@contextmanager
	def reg_mod():
		reg_mod = vi_timing.registry_create()
		yield reg_mod
		vi_timing.report(reg_mod)
		vi_timing.registry_close(reg_mod)

	print("\nTimes when using the C-extension:")
	print(f"\ttimeit.timeit(vi_timing.dummy): \t{timeit.timeit(vi_timing.dummy, number=1_000)*1_000:.2g} us")
	with reg_mod() as reg_mod:
		empty = vi_timing.create_measurement(reg_mod, "empty")
		def measure():
			s = vi_timing.get_ticks()
			f = vi_timing.get_ticks()
			vi_timing.add_measurement(empty, f - s)
		print(f"\tmeasure empty:\t{timeit.timeit(measure, number=1_000_000):.2g} us")
		dummy = vi_timing.create_measurement(reg_mod, "dummy")
		def measure():
			s = vi_timing.get_ticks()
			vi_timing.dummy()
			f = vi_timing.get_ticks()
			vi_timing.add_measurement(dummy, f - s)
		print(f"\tmeasure dummy:\t{timeit.timeit(measure, number=1_000_000):.2g} us")

	@contextmanager
	def reg_lib():
		reg_lib = lib.vi_tmRegistryCreate()
		yield reg_lib
		callback_addr = ctypes.cast(lib.vi_tmReportCb, ctypes.c_void_p).value
		lib.vi_tmReport(reg_lib, 320, callback_addr, None)
		lib.vi_tmRegistryClose(reg_lib)

	print("\nTimes when using exported functions via ctypes:")
	print(f"\ttimeit.timeit(lib.vi_Dummy): \t{timeit.timeit(lib.vi_Dummy, number=1_000)*1_000:.2g} us")
	with reg_lib() as reg_lib:
		empty = lib.vi_tmRegistryGetMeas(reg_lib, "empty".encode("utf-8"))
		def measure():
			s = lib.vi_tmGetTicks()
			f = lib.vi_tmGetTicks()
			lib.vi_tmMeasurementAdd(empty, f - s, 1)
		print(f"\tmeasure empty:\t{timeit.timeit(measure, number=1_000_000):.2g} us")
		dummy = lib.vi_tmRegistryGetMeas(reg_lib, "dummy".encode("utf-8"))
		def measure():
			s = lib.vi_tmGetTicks()
			lib.vi_Dummy();
			f = lib.vi_tmGetTicks()
			lib.vi_tmMeasurementAdd(dummy, f - s, 1)
		print(f"\tmeasure dummy:\t{timeit.timeit(measure, number=1_000_000):.2g} us")

	global_f_mod = vi_timing.get_ticks()
	global_f_lib = lib.vi_tmGetTicks()
	vi_timing.add_measurement(global_mod, global_f_mod - global_s_mod)
	lib.vi_tmMeasurementAdd(global_lib, global_f_lib - global_s_lib, 1)

	print("")
