# vi_timing.py

import ctypes
import importlib
import importlib.machinery
import importlib.util
import os
import sys
import timeit
from ctypes import (
	string_at, c_void_p, c_char_p, c_int32, c_uint32, c_uint64, 
#    Structure, c_double,
#    c_size_t, c_int32, POINTER, CFUNCTYPE, byref, create_string_buffer
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
vi_timing_dll = ctypes.CDLL(module_path)

###############################################################################
vi_timing_dll.vi_tmStaticInfo.argtypes = [c_uint32]
vi_timing_dll.vi_tmStaticInfo.restype = c_void_p
def vi_tmStaticInfo() -> str:
	return string_at(vi_timing_dll.vi_tmStaticInfo(c_uint32(2))).decode("utf-8")

vi_timing_dll.vi_tmDummy.argtypes = []
vi_timing_dll.vi_tmDummy.restype = None
def vi_tmDummy():
	vi_timing_dll.vi_tmDummy()

vi_timing_dll.vi_tmSleep.argtypes = [c_uint32]
vi_timing_dll.vi_tmSleep.restype = None
def vi_tmSleep(ms: c_uint32):
	vi_timing_dll.vi_tmSleep(ms)

###############################################################################
# Provide a simple CLI demo entrypoint
if __name__ == "__main__":
	print(f"vi_timing.version: \t{vi_timing.version()}")
	print(f"vi_tmStaticInfo: \t{vi_tmStaticInfo()}")

	print("\nTiming")
	print(f"vi_timing.version: \t{timeit.timeit(vi_timing.version, number=1_000)*1_000:.2g} us")
	print(f"vi_timing_dll.vi_tmStaticInfo: \t{timeit.timeit(lambda: string_at(vi_timing_dll.vi_tmStaticInfo(c_uint32(2))).decode("utf-8"), number=1_000)*1_000:.2g} us")
	print(f"vi_tmStaticInfo: \t{timeit.timeit(vi_tmStaticInfo, number=1_000)*1_000:.2g} us")
	print("")
	print(f"vi_timing.dummy: \t{timeit.timeit(vi_timing.dummy, number=1_000)*1_000:.2g} us")
	print(f"vi_timing_dll.vi_tmDummy: \t{timeit.timeit(vi_timing_dll.vi_tmDummy, number=1_000)*1_000:.2g} us")
	print(f"vi_tmDummy: \t\t{timeit.timeit(vi_tmDummy, number=1_000)*1_000:.2g} us")
	print("")
	print(f"vi_timing.sleep: \t{timeit.timeit(lambda: vi_timing.sleep(30), number=100)*10:.2g} ms")
	print(f"vi_timing_dll.vi_tmSleep: \t{timeit.timeit(lambda: vi_timing_dll.vi_tmSleep(30), number=100)*10:.2g} ms")
	print(f"vi_tmSleep: \t\t{timeit.timeit(lambda: vi_tmSleep(30), number=100)*10:.2g} ms")
	print("")

	vi_timing.dummy()
	vi_timing.sleep(30)
