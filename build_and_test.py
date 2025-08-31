#!/usr/bin/env python3

import argparse
import datetime
import itertools
import os
import shlex
import shutil
import stat
import subprocess

def get_cmake_property(key: str) -> str | None:
	"""
	Get the value of a CMake system property from `cmake --system-information`.

	:param key: The property name (e.g., "CMAKE_CXX_COMPILER_ID").
	:return: The value of the property as a string, or None if not found.
	"""

	try:
		result = subprocess.run(
			["cmake", "--system-information"],
			stdout=subprocess.PIPE,
			stderr=subprocess.PIPE,
			encoding="utf-8",
			text=True,
			check=True,
		)

	except FileNotFoundError:
		raise RuntimeError("CMake not found in PATH")
	except subprocess.CalledProcessError as e:
		raise RuntimeError(f"Error running cmake: {e.stderr.strip()}")

	for line in result.stdout.splitlines():
		if key in line:
			# support both "key: value" and 'key "value"' formats
			parts = line.split(":", 1)
			if len(parts) == 2:
				return parts[1].strip().strip('"')
			parts = shlex.split(line)
			if len(parts) >= 2:
				return parts[1]
	return None

def remove_readonly(func, path, _):
    os.chmod(path, stat.S_IWRITE)
    func(path)

def run(cmd, cwd=None):
	print(f"RUN: {' '.join(cmd)} (cwd={cwd or os.getcwd()})")
	result = subprocess.run(cmd, cwd=cwd, check=True)

def parse_params()->argparse.Namespace:
	project_root = os.path.dirname(__file__) if '__file__' in globals() else os.getcwd()
	test_root = os.path.join(project_root, "_test")

	parser = argparse.ArgumentParser(description="Automate testing for all CMake option combinations")
	parser.add_argument(
		"-S",
		"--path-to-source",
		default=project_root,
		help="Explicitly specify a source directory."
	)
	parser.add_argument(
		"-B",
		"--path-to-build",
		default=test_root,
		help="Explicitly specify a build directory."
	)
	parser.add_argument(
		"params",
		nargs="*",
		help='Build suffixes (e.g. "rfm"). If omitted, all combos will be tested.'
	)

	return parser.parse_args()

OPTIONS = [
	("VI_TM_STAT_USE_RAW", 'r'),
	("VI_TM_STAT_USE_FILTER", 'f'),
	("VI_TM_STAT_USE_MINMAX", 'm'),
	("VI_TM_THREADSAFE", 't'),
	("BUILD_SHARED_LIBS", 's'),
#    ("VI_TM_DEBUG", 'd'),
]

def get_pair(combo)-> tuple[str, list[str]]:
	suffix = ""
	params = []
	for opt, flag in zip(OPTIONS, combo):
		suffix += opt[1] if flag else ""
		params.append(f"-D{opt[0]}={'ON' if flag else 'OFF'}")

	return suffix, params


def folder_prepare(path: str):
	if os.path.exists(path) and os.path.isdir(path):
		shutil.rmtree(path, onerror=remove_readonly)
	os.makedirs(path)

def filter_suffix(suffix: str, suffix_filters: str)->bool:
	return suffix_filters and suffix not in suffix_filters

def main():
	start_all = datetime.datetime.now()
	print(f"[START ALL]: {start_all.strftime("%H:%M:%S")}")

	PARAMS = parse_params()

	path_to_source = PARAMS.path_to_source
	test_root = PARAMS.path_to_build
	suffix_filters = PARAMS.params

	folder_prepare(test_root)

	for combo in itertools.product([False, True], repeat=len(OPTIONS)):
		suffix, params = get_pair(combo)
		if filter_suffix(suffix, suffix_filters):
			continue

		params += ["-DCMAKE_BUILD_TYPE=Release"]
		name = "vi_timing_" + suffix

		start = datetime.datetime.now()
		print(f"[START] {name}: {start.strftime("%H:%M:%S")}")

		build_dir = os.path.join(test_root, name)
		os.makedirs(build_dir)

		print("Configuring CMake:")
		cmake_args = ["cmake", "-S", path_to_source, "-B", build_dir] + params
		run(cmake_args)
		print("Configuring CMake - done\n")

		print("Build the project:")
		run(["cmake", "--build", build_dir, "--config", "Release"])
		print("Build the project - done\n")

		# Run the tests; the script will terminate upon the first error.

		params = ["ctest", "--test-dir", build_dir, "--output-on-failure"]
		match get_cmake_property("CMAKE_CXX_COMPILER_ID"):
			case "GNU" | "Clang":
				None
			case "MSVC":
				params += ["--build-config", "Release"]
			case _:
				print("Warning: Unknown compiler, skipping tests.")

		print("Run the tests:")
		run(params)
		print("Run the tests - done")

		print(f"[FINISH] {name} [{(datetime.datetime.now() - start).total_seconds():.3f}s]\n")

	print("All combinations have been successfully assembled and tested.")
	print(f"[FINISH ALL] [{(datetime.datetime.now() - start_all).total_seconds():.3f}s].\n")

if __name__ == "__main__":
	main()
