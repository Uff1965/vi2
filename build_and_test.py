#!/usr/bin/env python3

import argparse
import datetime
import itertools
import os
import pathlib
import shlex
import shutil
import stat
import subprocess
import sys

def format_duration(seconds: float) -> str:
	hours, secs = divmod(round(seconds, 1), 3600)
	hours = int(hours)
	minutes, secs = divmod(secs, 60)
	minutes = int(minutes)

	parts = []
	if hours > 0:
		parts.append(f"{hours}h")
		parts.append(f"{minutes:02d}m")
	elif minutes > 0:
		parts.append(f"{minutes}m")
		parts.append(f"{int(secs):02d}s")
	else:
		parts.append(f"{float(secs):.1f}s")
	return ' '.join(parts)

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

PROJECT_ROOT = os.path.dirname(__file__) if '__file__' in globals() else os.getcwd()
EMPTY = "__EMPTY__"

def parse_params()->argparse.Namespace:
	parser = argparse.ArgumentParser(
		description="A utility for automatically configuring, building, and testing the viTiming library with different combinations of options.",
		epilog = "Example usage:\n\tpython build_and_test.py -S . -B _tests -T _bin -C Release rf '\"\"'",
		formatter_class=argparse.ArgumentDefaultsHelpFormatter
	)
	parser.add_argument(
		"-S",
		"--path-to-source",
		type=pathlib.Path,
		metavar="<dir>",
		default=PROJECT_ROOT,
		help="Explicitly specify a source directory."
	)
	parser.add_argument(
		"-B",
		"--path-to-build",
		type=pathlib.Path,
		metavar="<dir>",
		default="_tests",
		help="Explicitly specify a build directory."
	)
	parser.add_argument(
		"-T",
		"--path-to-result",
		type=pathlib.Path,
		metavar="<dir>",
		default="bin",
		help="Explicitly specify a target directory."
	)
	parser.add_argument(
		"-C",
		"--build-config",
		type=str,
		metavar="<config>",
		default="Release",
		help="Choose configuration to test."
	)
	parser.add_argument(
		"filters",
		type=str,
		nargs="*",
		help=f"Build suffixes (e.g. 'rfm'). '{EMPTY}' or '\"\"' to pass an empty filter to Windows CMD. If omitted, all combos will be tested."
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

def folder_remake(path: str):
	if os.path.exists(path):
		shutil.rmtree(path, onerror=remove_readonly)
	os.makedirs(path)

def skip_suffix(suffix: str, filters: list[str]) -> bool:
	if not filters:
		return False

	suffix_set = set(suffix)

	if not suffix_set:
		return all(f for f in filters)

	return not any(f and set(f) <= suffix_set for f in filters)

def main():
	start_all = datetime.datetime.now()
	print(f"[START ALL]: {start_all.strftime('%H:%M:%S')}")
	print(f"Agrs: {sys.argv}")
	print()

	PARAMS = parse_params()
	
	path_to_source = PARAMS.path_to_source
	if not os.path.isabs(path_to_source):
		path_to_source = os.path.abspath(os.path.join(PROJECT_ROOT, path_to_source))

	test_root = PARAMS.path_to_build
	if not os.path.isabs(test_root):
		test_root = os.path.abspath(os.path.join(PROJECT_ROOT, test_root))
	folder_remake(test_root)

	path_to_result = PARAMS.path_to_result
	if not os.path.isabs(path_to_result):
		path_to_result = os.path.abspath(os.path.join(test_root, path_to_result))
	folder_remake(path_to_result)

	suffix_filters = PARAMS.filters
	suffix_filters = ["" if item == EMPTY else item for item in suffix_filters]

	build_config = PARAMS.build_config;

	counter = 0
	for combo in itertools.product([False, True], repeat=len(OPTIONS)):
		suffix, options = get_pair(combo)
		if skip_suffix(suffix, suffix_filters):
			continue

		counter += 1
		name = "vi_timing_" + suffix

		start = datetime.datetime.now()
		print(f"[START] {name}: {start.strftime('%H:%M:%S')}")

		build_dir = os.path.join(test_root, "_build_" + suffix)
		print(f"build_dir: \'{build_dir}\'")
		if os.path.exists(build_dir) and os.path.isdir(build_dir):
			shutil.rmtree(build_dir, onerror=remove_readonly)


		print("Configuring CMake:")
		params = ["cmake", "-S", str(path_to_source), "-B", str(build_dir), f"-DCMAKE_BUILD_TYPE={build_config}", f"-DVI_TM_OUTPUT_PATH={str(path_to_result)}"]
		params += options
		run(params)
		print("Configuring CMake - done\n")

		print("Build the project:")
		run(["cmake", "--build", str(build_dir), "--config", build_config])
		print("Build the project - done\n")

		# Run the tests; the script will terminate upon the first error.

		params = ["ctest", "--test-dir", str(build_dir), "--output-on-failure"]
		match get_cmake_property("CMAKE_CXX_COMPILER_ID"):
			case "GNU" | "Clang":
				None
			case "MSVC":
				params += ["--build-config", build_config]
			case _:
				print("Warning: Unknown compiler, skipping tests.")

		print("Run the tests:")
		run(params)
		print("Run the tests - done")

		shutil.rmtree(build_dir, onerror=remove_readonly)
		print(f"[FINISH] {name} [Elapsed: {format_duration((datetime.datetime.now() - start).total_seconds())}]\n")

	print(f"All {counter} combinations have been successfully assembled and tested.")
	print()
	print("Reminder:")
	print(f"Source directory: \'{path_to_source}\'.")
	print(f"Build directory: \'{test_root}\'.")
	print(f"Target directory: \'{path_to_result}\'.")
	print()
	print(f"[FINISH ALL] [Elapsed: {format_duration((datetime.datetime.now() - start_all).total_seconds())}].\n")

if __name__ == "__main__":
	main()
