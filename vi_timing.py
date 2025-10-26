# vi_timing_py.py
from ctypes import (
    CDLL, Structure, c_uint32, c_uint64, c_size_t, c_double,
    c_char_p, c_void_p, c_int32, POINTER, CFUNCTYPE, byref, create_string_buffer
)
from ctypes.util import find_library
import functools
import os
import sys
import time
import timeit

# Attempt to find and load the shared library
def _load_library(name_hint="vi_timing"):
    candidates = []

    # Add current directory (where this file resides) to search candidates
    here = os.path.dirname(os.path.realpath(__file__))
    if here:    
        candidates.append(os.path.join(here, name_hint))

    # allow explicit environment override
    env = os.environ.get("VI_TIMING_LIB")
    if env:
        candidates.append(env)

    # try system find
    libname = find_library(name_hint)
    if libname:
        candidates.append(libname)

    # common platform names
    if sys.platform == "win32":
        candidates.append(name_hint)
    elif sys.platform == "darwin":
        candidates.extend([f"lib{name_hint}.dylib", f"{name_hint}.dylib"])
    else:
        candidates.extend([f"lib{name_hint}.so", f"{name_hint}.so"])

    last_exc = None
    for c in candidates:
        if not c:
            continue
        try:
            return CDLL(c)
        except Exception as e:
            last_exc = e
    raise RuntimeError(f"Failed to load '{name_hint}' library. Tried: {candidates}. Last error: {last_exc}")

_lib = _load_library("vi_timing")

# Type aliases matching header
VI_TM_RESULT = c_int32
VI_TM_FLAGS = c_uint32
VI_TM_FP = c_double
VI_TM_SIZE = c_size_t
VI_TM_TICK = c_uint64
VI_TM_TDIFF = VI_TM_TICK
VI_TM_HMEAS = c_void_p
VI_TM_HJOUR = c_void_p

# Callback type for report: VI_SYS_CALL  vi_tmReportCb_t(const char* str, void* ctx)
ReportCbType = CFUNCTYPE(VI_TM_RESULT, c_char_p, c_void_p)

# Define vi_tmStats_t according to header (common default configuration)
# If your library was built with different macros, adjust this structure accordingly.
class vi_tmStats_t(Structure):
    _fields_ = [
        ("calls_", VI_TM_SIZE),
        # VI_TM_STAT_USE_RAW
        ("cnt_", VI_TM_SIZE),
        ("sum_", VI_TM_TDIFF),
        # VI_TM_STAT_USE_RMSE
        ("flt_calls_", VI_TM_SIZE),
        ("flt_cnt_", VI_TM_FP),
        ("flt_avg_", VI_TM_FP),
        ("flt_ss_", VI_TM_FP),
        # VI_TM_STAT_USE_MINMAX (optional); keep them here � if not used by library they may be unused padding
#        ("min_", VI_TM_FP),
#        ("max_", VI_TM_FP),
    ]

    def as_dict(self):
        return {
            "calls": int(self.calls_),
            "cnt": int(self.cnt_),
            "sum": int(self.sum_),
            "flt_calls": int(self.flt_calls_),
            "flt_cnt": float(self.flt_cnt_),
            "flt_avg": float(self.flt_avg_),
            "flt_ss": float(self.flt_ss_),
#            "min": float(self.min_),
#            "max": float(self.max_),
        }

# Bind library functions with argtypes/restype
_lib.vi_tmGetTicks.restype = VI_TM_TICK
_lib.vi_tmGetTicks.argtypes = []

_lib.vi_tmInit.restype = VI_TM_RESULT
_lib.vi_tmInit.argtypes = [c_char_p, VI_TM_FLAGS, VI_TM_FLAGS]

_lib.vi_tmShutdown.restype = None
_lib.vi_tmShutdown.argtypes = []

_lib.vi_tmRegistryCreate.restype = VI_TM_HJOUR
_lib.vi_tmRegistryCreate.argtypes = []

_lib.vi_tmRegistryClose.restype = None
_lib.vi_tmRegistryClose.argtypes = [VI_TM_HJOUR]

_lib.vi_tmRegistryGetMeas.restype = VI_TM_HMEAS
_lib.vi_tmRegistryGetMeas.argtypes = [VI_TM_HJOUR, c_char_p]

_lib.vi_tmMeasurementAdd.restype = None
_lib.vi_tmMeasurementAdd.argtypes = [VI_TM_HMEAS, VI_TM_TDIFF, VI_TM_SIZE]

_lib.vi_tmMeasurementGet.restype = None
_lib.vi_tmMeasurementGet.argtypes = [VI_TM_HMEAS, POINTER(c_char_p), POINTER(vi_tmStats_t)]

_lib.vi_tmMeasurementReset.restype = None
_lib.vi_tmMeasurementReset.argtypes = [VI_TM_HMEAS]

_lib.vi_tmStatsReset.restype = None
_lib.vi_tmStatsReset.argtypes = [POINTER(vi_tmStats_t)]

_lib.vi_tmReport.restype = VI_TM_RESULT
_lib.vi_tmReport.argtypes = [VI_TM_HJOUR, VI_TM_FLAGS, ReportCbType, c_void_p]

# Optional convenience: global registry handle macro value used in header: VI_TM_HGLOBAL ((VI_TM_HJOUR)-1)
VI_TM_HGLOBAL = c_void_p(~0)  # (void*)-1, same idea

# Python wrapper API

def init(title: str = b"Timing report:\n", report_flags: int = 0x140, flags: int = 0) -> int:
    """Initialize vi_timing. title can be bytes or str."""
    if isinstance(title, str):
        title = title.encode("utf-8")
    return int(_lib.vi_tmInit(title, VI_TM_FLAGS(report_flags), VI_TM_FLAGS(flags)))

def shutdown() -> None:
    """Shutdown vi_timing."""
    _lib.vi_tmShutdown()

def registry_create():
    """Create and return a new registry handle (opaque)."""
    return _lib.vi_tmRegistryCreate()

def registry_close(registry):
    """Close a registry handle returned by registry_create."""
    _lib.vi_tmRegistryClose(registry)

def registry_get_meas(registry, name: str):
    """Get (or create) a measurement handle by name in the registry."""
    if isinstance(name, str):
        name = name.encode("utf-8")
    return _lib.vi_tmRegistryGetMeas(registry, c_char_p(name))

def measurement_add(meas, duration_ticks: int, count: int = 1):
    """Add measured duration (in ticks) to a measurement handle."""
    _lib.vi_tmMeasurementAdd(meas, VI_TM_TDIFF(duration_ticks), VI_TM_SIZE(count))

def measurement_get(meas):
    """Return (name, stats_dict) for measurement handle."""
    name_ptr = c_char_p()
    stats = vi_tmStats_t()
    _lib.vi_tmMeasurementGet(meas, byref(name_ptr), byref(stats))
    name = None
    if name_ptr:
        name = name_ptr.value.decode("utf-8")
    return name, stats.as_dict()

def stats_reset(stats=None):
    """Reset vi_tmStats_t (pass a vi_tmStats_t instance or returns a new zeroed one)."""
    if stats is None:
        s = vi_tmStats_t()
        _lib.vi_tmStatsReset(byref(s))
        return s
    else:
        _lib.vi_tmStatsReset(byref(stats))
        return stats

# Report wrapper: collect report into Python callable
def vi_tmReport(registry=VI_TM_HGLOBAL, flags: int = 0):
    """Generate report for registry. Returns whole report as a str."""
    chunks = []

    @ReportCbType
    def _cb(s: c_char_p, ctx):
        if s:
            try:
                chunks.append(s.decode("utf-8", errors="replace"))
            except Exception:
                chunks.append("<decode error>")
        return 0

    _lib.vi_tmReport(registry, VI_TM_FLAGS(flags), _cb, None)
    return "".join(chunks)

# High-level helpers: context manager and decorator for Python timing
class ViTimer:
    """Context manager that measures elapsed time and reports it to vi_timing.

    Usage:
        with ViTimer("my.measurement"):
            do_work()
    """

    def __init__(self, name: str, count: int = 1, registry=VI_TM_HGLOBAL):
        self.name = name
        self.count = count
        self.registry = registry
        self._meas = None
        self._start = None

    def __enter__(self):
        self._meas = registry_get_meas(self.registry, self.name)
        # start using host monotonic clock but translate to library ticks:
        # we capture vi_tmGetTicks at enter and exit and send diff in ticks.
        self._start = int(_lib.vi_tmGetTicks())
        return self

    def __exit__(self, exc_type, exc, tb):
        end = int(_lib.vi_tmGetTicks())
        dur = end - self._start
        measurement_add(self._meas, dur, self.count)
        return False  # do not suppress exceptions

def profile_function(name: str = None, registry=VI_TM_HGLOBAL):
    """Decorator that records elapsed time of the wrapped function into a named measurement.

    If name is None, the measurement name will be "<module>.<qualname>".
    """

    def deco(fn):
        nm = name or f"{fn.__module__}.{fn.__qualname__}"

        @functools.wraps(fn)
        def wrapper(*args, **kwargs):
            meas = registry_get_meas(registry, nm)
            start = int(_lib.vi_tmGetTicks())
            try:
                return fn(*args, **kwargs)
            finally:
                end = int(_lib.vi_tmGetTicks())
                measurement_add(meas, end - start, 1)
        return wrapper
    return deco

def vi_tmInfoVersion() -> str:
    """Get vi_timing version string."""
    _lib.vi_tmStaticInfo.restype = c_char_p
    _lib.vi_tmStaticInfo.argtypes = [VI_TM_FLAGS]
    VI_TM_INFO_VERSION = 2
    return _lib.vi_tmStaticInfo(VI_TM_INFO_VERSION).decode("utf-8")

# Example convenience: simple timing of Python callable using monotonic -> ticks translation fallback
def vi_tmInfoUnit() -> float:
    """Try to read vi_timing unit info via vi_tmStaticInfo if available.
    Fallback: we guess 1e9 ticks per second if not available (common for ns ticks).
    """
    # Try to find vi_tmStaticInfo in library
    try:
        _lib.vi_tmStaticInfo.restype = c_void_p
        _lib.vi_tmStaticInfo.argtypes = [VI_TM_FLAGS]
        # vi_tmInfoUnit returns const double* (seconds per tick)
        # We pass vi_tmInfoUnit enum value (named in header as vi_tmInfoUnit) which is 7 in header's order.
        VI_TM_INFO_UNIT = 7
        p = _lib.vi_tmStaticInfo(VI_TM_INFO_UNIT)
        if p:
            # cast to pointer to double
            dbl_ptr = POINTER(c_double).from_address(p)
            unit = float(dbl_ptr[0])
            if unit and unit > 0:
                return 1.0 / unit
    except Exception:
        pass
    return 1e9  # guessed ticks per second (nanosecond resolution)

def tick_pair_and_add(meas: VI_TM_HMEAS):
    start = _lib.vi_tmGetTicks()
    end = _lib.vi_tmGetTicks()
    dur = end - start
    _lib.vi_tmMeasurementAdd(meas, dur, VI_TM_SIZE(1))

# Provide a simple CLI demo entrypoint
if __name__ == "__main__":
    print(f"vi_timing version: {vi_tmInfoVersion()}\n")

    init()
    # demo: profile a few sleep calls
    jnl = registry_create()
    with ViTimer(f"sleep", 4, jnl):
        for i in range(1, 5):
            with ViTimer(f"sleep-{i}") as _:
                time.sleep(0.01 * (i))

    cnt = 1_000_000
    meas_it = registry_get_meas(jnl, "for timeit")
    with ViTimer(f"for timeit total", cnt, jnl):
        timer = timeit.Timer(functools.partial(tick_pair_and_add, meas_it))
    print(f"tick_pair_and_add ({cnt}): {timer.timeit(number=cnt)} us\n")

    out = vi_tmReport(jnl)
    print("Report local registry:\n")
    print(out)

    registry_close(jnl)
    shutdown()
