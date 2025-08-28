#!/usr/bin/env python3

import itertools
import os
import shutil
import subprocess
import sys

def run(cmd, cwd=None):
    print(f"[RUN] {' '.join(cmd)} (cwd={cwd or os.getcwd()})")
    result = subprocess.run(cmd, cwd=cwd, check=True)
    if result.returncode != 0:
        sys.exit(result.returncode)

OPTIONS = [
    ("VI_TM_STAT_USE_RAW", 'r'),
    ("VI_TM_STAT_USE_FILTER", 'f'),
    ("VI_TM_STAT_USE_MINMAX", 'm'),
    ("VI_TM_THREADSAFE", 't'),
    ("BUILD_SHARED_LIBS", 's'),
#    ("VI_TM_DEBUG", 'd'),
]

def main():
    project_root = os.path.abspath(os.path.dirname(__file__))
    test_root = os.path.join(project_root, "_test")

    # Recreate the _test folder
    if os.path.exists(test_root):
        shutil.rmtree(test_root)
    os.makedirs(test_root, exist_ok=True)

    for combo in itertools.product([False, True], repeat=len(OPTIONS)):
        name = "vi_timing_"
        params = []
        for opt, flag in zip(OPTIONS, combo):
            name += opt[1] if flag else ""
            params.append(f"-D{opt[0]}={'ON' if flag else 'OFF'}")

        print(f"[START] {name}")

        build_dir = os.path.join(test_root, name)
        os.makedirs(build_dir)

        # Configuring CMake
        cmake_args = ["cmake", project_root, "-DCMAKE_BUILD_TYPE=Release"] + params
        run(cmake_args, cwd=build_dir)

        # Build the project
        run(["cmake", "--build", ".", "--config", "Release"], cwd=build_dir)

        # Run the tests; the script will terminate upon the first error.
        run(["ctest", "-C", "Release", "--output-on-failure"], cwd=build_dir)
        print("[DONE] {name}", end="\n\n")

    print("All combinations have been successfully assembled and tested.")

if __name__ == "__main__":
#    try:
        main()
#    except Exception as e:
#        print("Fatal error:", e, file=sys.stderr)
#        sys.exit(1)
