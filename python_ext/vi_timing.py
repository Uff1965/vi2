# vi_timing.py

import ctypes
import importlib
import importlib.machinery
import importlib.util
import os
import sys
import timeit
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

	print("\nTiming")
	print(f"vi_timing.version: \t{timeit.timeit(vi_timing.version, number=1_000)*1_000:.2g} us")
	print(f"lib::vi_tmStaticInfo: \t{timeit.timeit(lambda: string_at(lib.vi_tmStaticInfo(c_uint32(2))).decode("utf-8"), number=1_000)*1_000:.2g} us")
	print("")
	print(f"vi_timing.dummy: \t{timeit.timeit(vi_timing.dummy, number=1_000)*1_000:.2g} us")
	print(f"lib::vi_Dummy: \t{timeit.timeit(lib.vi_Dummy, number=1_000)*1_000:.2g} us")
	print("")
	print(f"vi_timing.sleep: \t{timeit.timeit(lambda: vi_timing.sleep(30), number=100)*10:.2g} ms")
	print(f"lib::vi_Sleep: \t{timeit.timeit(lambda: lib.vi_Sleep(30), number=100)*10:.2g} ms")
	print("")

	reg_mod = vi_timing.registry_create()
	mes_mod = vi_timing.create_measurement(reg_mod, "mod")

	for n in range(1_000):
		s = vi_timing.get_ticks()
		f = vi_timing.get_ticks()
		vi_timing.add_measurement(mes_mod, f - s)

	# Call report without callback
	vi_timing.report(reg_mod)
	vi_timing.registry_close(reg_mod)

	print("")

	reg_lib = lib.vi_tmRegistryCreate()
	mes_lib = lib.vi_tmRegistryGetMeas(reg_lib, "lib".encode("utf-8"))

	for n in range(1_000):
		s = lib.vi_tmGetTicks()
		f = lib.vi_tmGetTicks()
		lib.vi_tmMeasurementAdd(mes_lib, f - s, 1)

	func_ptr = lib.vi_tmReportCb
	func_ptr_addr = ctypes.cast(func_ptr, ctypes.c_void_p).value

	# Call report with callback pointer
	lib.vi_tmReport(reg_lib, 320, func_ptr_addr, None)
	lib.vi_tmRegistryClose(reg_lib)

	vi_timing.dummy()
	vi_timing.sleep(30)
