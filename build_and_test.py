#!/usr/bin/env python3

"""
Automated build and test utility.
Performs full compilation of the target program and related examples \
across all valid combinations of options and configurations. Ensures \
correctness and compatibility of builds, streamlining debugging, \
regression checks, and coverage analysis. \
"""

import argparse # For command-line argument parsing
import datetime # For tracking build and test durations
import functools # For caching function results
import itertools # For generating combinations of options
import os # For operating system interactions
import pathlib # For filesystem path manipulations
import platform # For detecting the operating system
import shlex # For splitting command-line strings
import shutil # For high-level file operations
import stat # For file permission constants
import subprocess # For running external commands
import sys # For system-specific parameters and functions
from dataclasses import dataclass, field # For structured configuration storage

START_ALL_TIME = datetime.datetime.now() # Timestamp marking the start of all operations
PROJECT_ROOT:pathlib.Path = pathlib.Path(__file__).parent if '__file__' in globals() else pathlib.Path.cwd() # Root directory of the project
EMPTY = "__EMPTY__" # Placeholder for empty filter in command-line arguments
OPTIONS = [ # List of build options with their corresponding suffix characters
	("BUILD_SHARED_LIBS", 's'),
	("VI_TM_STAT_USE_RMSE", 'a'),
	("VI_TM_STAT_USE_FILTER", 'f'),
	("VI_TM_STAT_USE_MINMAX", 'm'),
	("VI_TM_STAT_USE_RAW", 'r'),
	("VI_TM_THREADSAFE", 't'),
]

@dataclass
class Config:
	"""Configuration dataclass to hold build and test settings."""
	path_to_source: pathlib.Path = PROJECT_ROOT # Default source directory
	path_to_build: pathlib.Path = "_tests" # Default build directory
	path_to_result: pathlib.Path = "bin" # Default output directory
	build_config: str = "Release" # Build configuration (Release/Debug)
	suffix_filters: list[str] = field(default_factory=list) # Filters for build suffixes
	cmake_defines: list[str] = field(default_factory=list) # Additional CMake definitions
	list_only: bool = False # Flag to only list combinations without building
	dry_run: bool = False # Flag to show commands without executing them
	build_count: int = 0 # Counter for the number of builds performed
	total: int = 0 # Total number of builds to perform

config: Config # Global configuration instance

# ---------------- Helpers ----------------

def make_relative_if_subpath(target: pathlib.Path, base: pathlib.Path = pathlib.Path.cwd()) -> pathlib.Path:
	"""Make path relative to base if it is a subpath; otherwise return absolute path."""
	target = target.resolve()
	base = base.resolve()
	return target.relative_to(base) if target.parts[:len(base.parts)] == base.parts else target

def folder_remake(path: pathlib.Path) -> None:
	"""Recreate a folder: remove if exists and create anew."""
	if not isinstance(path, pathlib.Path):
		path = pathlib.Path(path)

	if config.dry_run:
		return

	def _on_rm_error(func, path: str, _exc_info) -> None:
		"""Remove read-only attribute from a file and retry the operation."""
		pathlib.Path(path).chmod(stat.S_IWRITE)
		func(path)

	if path.exists():
		shutil.rmtree(path, onerror=_on_rm_error)

	# Create a fresh empty folder
	path.mkdir(parents=True, exist_ok=True)

def format_duration(seconds: float) -> str:
	"""Format duration in a human-readable form"""
	# Round to 1 decimal place for consistency
	total_seconds = round(seconds, 2)

	# Break down into days, hours, minutes, seconds
	days, remainder = divmod(total_seconds, 24 * 60 * 60)  # 24 * 60 * 60
	hours, remainder = divmod(remainder, 60 * 60)
	minutes, remainder = divmod(remainder, 60)

	# Convert to appropriate types
	days = int(days)
	hours = int(hours)
	minutes = int(minutes)
	secs = remainder  # float, may include decimal part

	parts = []
	if days > 0:
		parts.append(f"{days}d")
		parts.append(f"{hours:02d}h")
		parts.append(f"{minutes:02d}m")
	elif hours > 0:
		parts.append(f"{hours}h")
		parts.append(f"{minutes:02d}m")
		parts.append(f"{int(secs):02d}s")
	elif minutes > 0:
		parts.append(f"{minutes}m")
		parts.append(f"{secs:02.1f}s")
	else:
		parts.append(f"{secs:.2f}s")

	return ' '.join(parts)

@functools.cache
def load_cmake_system_info() -> list[str]:
	"""Load CMake system information and cache the result."""
	return subprocess.run(
		["cmake", "--system-information"],
		capture_output=True,
		encoding="utf-8",
		check=True,
	).stdout.splitlines()

def get_cmake_property(key: str) -> str:
	"""Get a specific CMake property from the system information."""
	key_len = len(key)

	for line in load_cmake_system_info():
		if len(line) > key_len and line.startswith(key) and line[key_len] in (":", " ", "="):
			return line[key_len + 1:].strip(' \'"').strip()
	raise KeyError(f"CMake property '{key}' not found")

def run(cmd: list[str], cwd: str | None = None)-> subprocess.CompletedProcess | None:
	"""Run a command and optionally print it."""
	print(f"RUN: {' '.join(cmd)}")
	return subprocess.CompletedProcess(cmd, returncode=0) if config.dry_run else subprocess.run(cmd, cwd=cwd, check=True)

def make_options(combo: tuple[bool,...])-> list[str]:
	"""Generate CMake options from a combination of boolean flags."""
	result = []
	for (name, _), use_flag in zip(OPTIONS, combo):
		result.append(f"-D{name}={'ON' if use_flag else 'OFF'}")
	return result

def make_suffix(flags: list[str]) -> str:
	"""Generate a suffix string based on enabled options and build configuration."""
	result = ""
	for name, char in OPTIONS:
		for flag in flags:
			if flag.startswith(f"-D{name}="):
				if flag[len(name) + 3:] == 'ON':
					result += char
				break

	if config.build_config.upper() == "DEBUG":
		result += "d"

	return result

def make_setjobs()->list[str]:
	"""Generate parallel build flags based on the CMake generator."""
	generator = get_cmake_property("CMAKE_GENERATOR")
	if generator is not None:
		jobs = os.cpu_count() or 4
		if "Ninja" in generator:
			return [f"-j{jobs}"]
		if "Makefiles" in generator:
			return ["--", f"-j{jobs}"]
		if generator.startswith("Visual Studio") or "MSBuild" in generator:
			return ["--", f"/m:{jobs}"]
	return []

def skip_suffix(suffix: str, filters: list[str]) -> bool:
	"""Determine whether to skip a build suffix based on filters."""
	# Check dependency: VI_TM_STAT_USE_FILTER requires VI_TM_STAT_USE_RMSE
	option_info = {name: symbol for name, symbol in OPTIONS}
	rmse = option_info["VI_TM_STAT_USE_RMSE"]
	fltr = option_info["VI_TM_STAT_USE_FILTER"]
	if fltr in suffix and rmse not in suffix:
		return True
	# If no filters specified, don't skip anything (except invalid dependencies)
	if not filters:
		return False
	# Empty suffix case: skip only if there are non-empty filters
	if not suffix:
		return all(filters)

	suffix_set = set(suffix)
	return not any(f and set(f) <= suffix_set for f in filters)

def configuring(build_dir: str, options: list[str]):
	"""Configure the CMake project with specified options."""
	print(f"Configuration the project {config.build_count}/{config.total}:")
	params = ["cmake"]
	params += ["-S", str(make_relative_if_subpath(config.path_to_source))]
	params += ["-B", str(make_relative_if_subpath(build_dir))]
	params += [f"-DCMAKE_BUILD_TYPE={config.build_config}"]
	params += [f"-DVI_TM_OUTPUT_PATH={str(make_relative_if_subpath(config.path_to_result))}"]
	params += options
	for val in config.cmake_defines:
		params += [f"-D{val}"]
	run(params)
	print("Configuration the project - done\n")

def build(build_dir: str):
	"""Build the CMake project."""
	print(f"Build the project {config.build_count}/{config.total}:")
	params = ["cmake"]
	params += ["--build", str(make_relative_if_subpath(build_dir))]
	params += ["--config", config.build_config]
	params += make_setjobs() #must be last
	run(params)
	print("Build the project - done\n")

def testing(build_dir: str):
	"""Test the CMake project."""
	print(f"Test the project {config.build_count}/{config.total}:")
	params = ["ctest"]
	params += ["--test-dir", str(make_relative_if_subpath(build_dir))]
	params += ["--output-on-failure"]
	match get_cmake_property("CMAKE_CXX_COMPILER_ID"):
		case "GNU" | "Clang":
			None
		case "MSVC":
			params += ["--build-config", config.build_config]
		case _:
			print("Warning: Unknown compiler, skipping tests.")

	# Run the tests; the script will terminate upon the first error.
	run(params)
	print("Test the project - done\n")

def work(options: list[str]):
	"""Perform the full build and test cycle for a given set of options."""
	start = datetime.datetime.now()
	suffix = make_suffix(options)
	name = "vi_timing_" + suffix
	if config.list_only:
		print(f"{name}")
	else:
		print("*" * 70)
		print(f"[START] {config.build_count}/{config.total} {name}: {start.strftime('%H:%M:%S')}")

#			build_dir = (config.path_to_build / ("_build_" + suffix))
#			print(f"build_dir: \'{make_relative_if_subpath(build_dir)}\'")
		build_dir = (config.path_to_build / "_build")
		print()
		if not config.dry_run and build_dir.exists() and build_dir.isdir():
			shutil.rmtree(build_dir, onerror=remove_readonly)

		configuring(build_dir, options)
		build(build_dir)
		testing(build_dir)

		if not config.dry_run:
			shutil.rmtree(build_dir, onerror=remove_readonly)

		print(f"[FINISH] {config.build_count}/{config.total}: {name} ["
			f"Elapsed: {format_duration((datetime.datetime.now() - start).total_seconds())} "
			f"(all: {format_duration((datetime.datetime.now() - START_ALL_TIME).total_seconds())})"
			"]")
		print("*" * 70)
		print()

# ---------------- CLI ----------------

def parse_args() -> Config:
	"""Parse command-line arguments and return a Config instance."""
	parser = argparse.ArgumentParser(
		description=__doc__,
		epilog = "Example usage:\n\tpython build_and_test.py -S . -B _tests -T _bin -C Release rf '\"\"'",
		formatter_class=argparse.ArgumentDefaultsHelpFormatter
	)

	parser.add_argument("filters", type=str, nargs="*", help="Build suffixes (e.g. 'rfm'). "
			f"'{EMPTY}' or '\"\"' to pass an empty filter to Windows CMD. If omitted, all combos will be tested.")
	parser.add_argument("-S", "--path-to-source", type=pathlib.Path, metavar="<dir>", default=pathlib.Path("."), help="Explicitly specify a source directory.")
	parser.add_argument("-B", "--path-to-build", type=pathlib.Path, metavar="<dir>", default=pathlib.Path("_tests"), help="Explicitly specify a build directory.")
	parser.add_argument("-T", "--path-to-result", type=pathlib.Path, metavar="<dir>", default=pathlib.Path("bin"), help="Explicitly specify a target directory.")
	parser.add_argument("-C", "--build-config", choices=["Release", "Debug"], default="Release", help="Choose configuration to test.")
	parser.add_argument("-D", type=str, metavar="<var>[:<type>]=<value>", action="append", dest="cmake_defines", help="Create or update a cmake cache entry.")
	parser.add_argument("-v", "--verbose", action="store_true", help="Verbose output")
	parser.add_argument("--list-only", action="store_true", help="List all combinations without building")
	parser.add_argument("--dry-run", action="store_true", help="Show commands without executing them")

	args = parser.parse_args()

	source = args.path_to_source
	if not source.is_absolute():
		source = (PROJECT_ROOT / source).absolute()
	if not source.exists() or not source.is_dir():
		print(f"ERROR: Source directory '{source}' does not exist!")
		exit(1)

	build = args.path_to_build
	if not build.is_absolute():
		build = (PROJECT_ROOT / build).absolute()

	result = args.path_to_result
	if not result.is_absolute():
		result = (build / result).absolute()

	filters = ["" if f == EMPTY else f for f in args.filters]

	result = Config(
		path_to_source=source,
		path_to_build=build,
		path_to_result=result,
		build_config=args.build_config,
		suffix_filters=filters,
		cmake_defines=args.cmake_defines or [],
		list_only=args.list_only,
		dry_run=args.dry_run or args.list_only
		)
	return result

# ---------------- Main ----------------

def main():
	"""Main function to orchestrate the build and test process."""
	global config
	config = parse_args()

	configurations = []

	for combo in itertools.product([False, True], repeat=len(OPTIONS)):
		options = make_options(combo)
		suffix = make_suffix(options)
		if not skip_suffix(suffix, config.suffix_filters):
			configurations.append(options)

	config.total = len(configurations)

	print(f"[START ALL] {config.total} combinations: {START_ALL_TIME.strftime('%H:%M:%S')}")
	print(f"Agrs: {sys.argv}")
	print(f"CWD = {pathlib.Path.cwd()}")
	print(f"Source directory: \'{make_relative_if_subpath(config.path_to_source)}\'")
	print(f"Build directory: \'{make_relative_if_subpath(config.path_to_build)}\'")
	print(f"Output directory: \'{make_relative_if_subpath(config.path_to_result)}\'")
	print()

	folder_remake(config.path_to_build)
	folder_remake(config.path_to_result)

	# Iterate over all possible True/False combinations for each option
	for options in configurations:
		config.build_count += 1
		work(options)

	if config.list_only:
		print(f"A total of {config.build_count} combinations.")
	else:
		print(f"All {config.build_count} combinations have been successfully assembled and tested.")
	print()
	print("Reminder:")
	print(f"Source directory: \'{config.path_to_source}\'.")
	print(f"Build directory: \'{config.path_to_build}\'.")
	print(f"Output directory: \'{config.path_to_result}\'.")
	print()
	print(f"[FINISH ALL] [Elapsed: {format_duration((datetime.datetime.now() - START_ALL_TIME).total_seconds())}].\n")

if __name__ == "__main__":
	main()
