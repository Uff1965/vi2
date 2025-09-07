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
from typing import Counter

START_ALL = datetime.datetime.now()
PROJECT_ROOT = os.path.dirname(__file__) if '__file__' in globals() else os.getcwd()
EMPTY = "__EMPTY__"
OPTIONS = [
    ("BUILD_SHARED_LIBS", 's', True),
    ("VI_TM_DEBUG", 'd', False),
    ("VI_TM_STAT_USE_FILTER", 'f', True),
    ("VI_TM_STAT_USE_MINMAX", 'm', True),
    ("VI_TM_STAT_USE_RAW", 'r', True),
    ("VI_TM_THREADSAFE", 't', True),
]

counter = 0
build_config = "Release"
path_to_result = "bin"
path_to_source = PROJECT_ROOT
test_root = "_tests"
suffix_filters = []

def format_duration(seconds: float) -> str:
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

def get_cmake_property(key: str = "CMAKE_CXX_COMPILER_ID") -> str | None:
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
    os.chmod(path, stat.S_IWRITE)  # Change file permissions to writable
    func(path)                     # Retry the original removal function

def run(cmd, cwd=None):
    print(f"RUN: {' '.join(cmd)} (cwd={cwd or os.getcwd()})")
    result = subprocess.run(cmd, cwd=cwd, check=True)

def parse_params()->argparse.Namespace:
    global build_config
    global path_to_result
    global path_to_source
    global suffix_filters
    global test_root

    parser = argparse.ArgumentParser(
        description="A utility for automatically configuring, building, and testing the viTiming library with different combinations of options.",
        epilog = "Example usage:\n\tpython build_and_test.py -S . -B _tests -T _bin -C Release rf '\"\"'",
        formatter_class=argparse.ArgumentDefaultsHelpFormatter
    )
    parser.add_argument(
        "-S",
        "--path-to-source",
        type = pathlib.Path,
        metavar = "<dir>",
        default = path_to_source,
        help = "Explicitly specify a source directory."
    )
    parser.add_argument(
        "-B",
        "--path-to-build",
        type = pathlib.Path,
        metavar = "<dir>",
        default = test_root,
        help = "Explicitly specify a build directory."
    )
    parser.add_argument(
        "-T",
        "--path-to-result",
        type = pathlib.Path,
        metavar = "<dir>",
        default = path_to_result,
        help = "Explicitly specify a target directory."
    )
    parser.add_argument(
        "-C",
        "--build-config",
        type = str,
        choices=["Release", "Debug"],
        default = build_config,
        help = "Choose configuration to test."
    )
    parser.add_argument(
        "filters",
        type=str,
        nargs="*",
        help="Build suffixes (e.g. 'rfm'). "
            f"'{EMPTY}' or '\"\"' to pass an empty filter to Windows CMD. "
            "If omitted, all combos will be tested."
    )

    params = parser.parse_args()

    path_to_source = params.path_to_source
    if not os.path.isabs(path_to_source):
        path_to_source = os.path.abspath(os.path.join(PROJECT_ROOT, path_to_source))

    test_root = params.path_to_build
    if not os.path.isabs(test_root):
        test_root = os.path.abspath(os.path.join(PROJECT_ROOT, test_root))
    folder_remake(test_root)

    path_to_result = params.path_to_result
    if not os.path.isabs(path_to_result):
        path_to_result = os.path.abspath(os.path.join(test_root, path_to_result))
    folder_remake(path_to_result)

    suffix_filters = params.filters
    suffix_filters = ["" if item == EMPTY else item for item in suffix_filters]

    build_config = params.build_config;

def make_options(combo: tuple[bool,...])-> list[str]:
    result = []
    for (name, char, enabled), use_flag in zip(OPTIONS, combo):
        # Only include flags for options marked as enabled
        if enabled:
           result.append(f"-D{name}={'ON' if use_flag else 'OFF'}")
    return result

def make_suffix(flags: list[str]) -> str:
    # Set of values considered as "enabled"
    enabled_values = {"on", "true", "1", "yes", "enabled"}
    result = ""

    for name, char, _ in OPTIONS:
        name_lower = name.lower()

        for flag in flags:
            flag_lower = flag.lower()

            # Skip if the flag doesn't match the current option name
            if not flag_lower.startswith(f"-d{name_lower}"):
                continue

            # If the flag has a value, check if it's in the enabled set
            if "=" in flag_lower:
                _, value = flag_lower.split("=", 1)
                if value.strip() not in enabled_values:
                    break  # Explicitly disabled � skip this option

            # Add the corresponding character to the result
            result += char
            break  # Move to the next option after first match

    if build_config.upper() == "DEBUG":
        suffix += "d"

    return result

def folder_remake(path: str):
    # Remove the folder if it exists
    if os.path.exists(path):
        shutil.rmtree(path, onerror=remove_readonly)
    # Create a fresh empty folder
    os.makedirs(path)

def skip_suffix(suffix: str, filters: list[str]) -> bool:
    if not filters:
        return False

    suffix_set = set(suffix)
    if not suffix_set:
        return all(f for f in filters)

    return not any(f and set(f) <= suffix_set for f in filters)

def configuring(build_dir: str, options: list[str]):
        print("Configuring CMake:")
        params = ["cmake", "-S", str(path_to_source), "-B", str(build_dir), f"-DCMAKE_BUILD_TYPE={build_config}", f"-DVI_TM_OUTPUT_PATH={str(path_to_result)}"]
        params += options
        run(params)
        print("Configuring CMake - done\n")

def build(build_dir: str):
        print("Build the project:")
        run(["cmake", "--build", str(build_dir), "--config", build_config])
        print("Build the project - done\n")

def testing(build_dir: str):
        params = ["ctest", "--test-dir", str(build_dir), "--output-on-failure"]
        match get_cmake_property():
            case "GNU" | "Clang":
                None
            case "MSVC":
                params += ["--build-config", build_config]
            case _:
                print("Warning: Unknown compiler, skipping tests.")

        print("Run the tests:")
        # Run the tests; the script will terminate upon the first error.
        run(params)
        print("Run the tests - done")

def work(options: list[str]):
        start = datetime.datetime.now()
        suffix = make_suffix(options)
        name = "vi_timing_" + suffix
        print(f"[START] {name}: {start.strftime('%H:%M:%S')}")

        build_dir = os.path.join(test_root, "_build_" + suffix)
        print(f"build_dir: \'{build_dir}\'")
        if os.path.exists(build_dir) and os.path.isdir(build_dir):
            shutil.rmtree(build_dir, onerror=remove_readonly)

        configuring(build_dir, options)
        build(build_dir)
        testing(build_dir)

        shutil.rmtree(build_dir, onerror=remove_readonly)
        print(f"[FINISH] {counter}: {name} ["
            f"Elapsed: {format_duration((datetime.datetime.now() - start).total_seconds())} "
            f"(all: {format_duration((datetime.datetime.now() - START_ALL).total_seconds())})"
            "]\n")
        print("*" * 60)

def main():
    global counter

    print(f"[START ALL]: {START_ALL.strftime('%H:%M:%S')}")
    print(f"Agrs: {sys.argv}")
    print()

    parse_params()

    # Iterate over all possible True/False combinations for each option
    for combo in itertools.product([False, True], repeat=len(OPTIONS)):
        # Skip combinations that try to enable a flag marked as disabled
        if any(use_flag and not enabled for (_, _, enabled), use_flag in zip(OPTIONS, combo)):
            continue

        options = make_options(combo)
        suffix = make_suffix(options)

        if skip_suffix(suffix, suffix_filters):
            continue

        counter += 1
        work(options)

    print(f"All {counter} combinations have been successfully assembled and tested.")
    print()
    print("Reminder:")
    print(f"Source directory: \'{path_to_source}\'.")
    print(f"Build directory: \'{test_root}\'.")
    print(f"Target directory: \'{path_to_result}\'.")
    print()
    print(f"[FINISH ALL] [Elapsed: {format_duration((datetime.datetime.now() - START_ALL).total_seconds())}].\n")

if __name__ == "__main__":
    main()
