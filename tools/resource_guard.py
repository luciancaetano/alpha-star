"""Hard caps on everything the harness launches, so a runaway build/test can never freeze the PC.

Windows: puts the current process (and every child it starts) into a Job Object with
  - a memory cap for the whole tree (default 8 GB)   -> the OS kills it instead of swapping to death
  - a hard CPU rate cap (default 50% of the machine)  -> the desktop stays responsive
  - below-normal priority
  - kill-on-close: when this process dies (even if its parent crashes), the OS kills the whole tree
Other OSes: lowers priority (nice) and caps address space with RLIMIT_AS.

Usage:
  import resource_guard; resource_guard.limit()          # first thing in a tool's main()
  python tools/resource_guard.py -- <command...>          # run any command under the caps
Env overrides: ALPHA_GUARD_MEM_GB, ALPHA_GUARD_CPU_PERCENT, ALPHA_GUARD_OFF=1 (never on a user's PC).
"""
import os
import subprocess
import sys

DEFAULT_MEM_GB = 8
DEFAULT_CPU_PERCENT = 50
_JOB = None  # keep the job handle alive for the life of this process


def _settings():
    mem_gb = float(os.environ.get("ALPHA_GUARD_MEM_GB", DEFAULT_MEM_GB))
    cpu = int(os.environ.get("ALPHA_GUARD_CPU_PERCENT", DEFAULT_CPU_PERCENT))
    return int(mem_gb * 1024 ** 3), max(1, min(cpu, 100))


def _windows_job(mem_bytes, cpu_percent):
    import ctypes
    from ctypes import wintypes

    k32 = ctypes.WinDLL("kernel32", use_last_error=True)

    class IO_COUNTERS(ctypes.Structure):
        _fields_ = [(n, ctypes.c_ulonglong) for n in (
            "ReadOperationCount", "WriteOperationCount", "OtherOperationCount",
            "ReadTransferCount", "WriteTransferCount", "OtherTransferCount")]

    class BASIC(ctypes.Structure):
        _fields_ = [("PerProcessUserTimeLimit", ctypes.c_longlong), ("PerJobUserTimeLimit", ctypes.c_longlong),
                    ("LimitFlags", wintypes.DWORD), ("MinimumWorkingSetSize", ctypes.c_size_t),
                    ("MaximumWorkingSetSize", ctypes.c_size_t), ("ActiveProcessLimit", wintypes.DWORD),
                    ("Affinity", ctypes.c_size_t), ("PriorityClass", wintypes.DWORD),
                    ("SchedulingClass", wintypes.DWORD)]

    class EXTENDED(ctypes.Structure):
        _fields_ = [("BasicLimitInformation", BASIC), ("IoInfo", IO_COUNTERS),
                    ("ProcessMemoryLimit", ctypes.c_size_t), ("JobMemoryLimit", ctypes.c_size_t),
                    ("PeakProcessMemoryUsed", ctypes.c_size_t), ("PeakJobMemoryUsed", ctypes.c_size_t)]

    class CPU_RATE(ctypes.Structure):
        _fields_ = [("ControlFlags", wintypes.DWORD), ("CpuRate", wintypes.DWORD)]

    JOB_OBJECT_LIMIT_PRIORITY_CLASS = 0x20
    JOB_OBJECT_LIMIT_JOB_MEMORY = 0x200
    JOB_OBJECT_LIMIT_KILL_ON_JOB_CLOSE = 0x2000
    BELOW_NORMAL_PRIORITY_CLASS = 0x4000
    JobObjectExtendedLimitInformation = 9
    JobObjectCpuRateControlInformation = 15
    CPU_RATE_ENABLE_HARD_CAP = 0x1 | 0x4

    k32.CreateJobObjectW.restype = wintypes.HANDLE
    k32.CreateJobObjectW.argtypes = [ctypes.c_void_p, wintypes.LPCWSTR]
    k32.SetInformationJobObject.argtypes = [wintypes.HANDLE, ctypes.c_int, ctypes.c_void_p, wintypes.DWORD]
    k32.AssignProcessToJobObject.argtypes = [wintypes.HANDLE, wintypes.HANDLE]
    k32.GetCurrentProcess.restype = wintypes.HANDLE
    job = k32.CreateJobObjectW(None, None)
    if not job:
        raise OSError(ctypes.get_last_error(), "CreateJobObject failed")
    info = EXTENDED()
    info.BasicLimitInformation.LimitFlags = (JOB_OBJECT_LIMIT_JOB_MEMORY | JOB_OBJECT_LIMIT_KILL_ON_JOB_CLOSE
                                             | JOB_OBJECT_LIMIT_PRIORITY_CLASS)
    info.BasicLimitInformation.PriorityClass = BELOW_NORMAL_PRIORITY_CLASS
    info.JobMemoryLimit = mem_bytes
    if not k32.SetInformationJobObject(job, JobObjectExtendedLimitInformation, ctypes.byref(info),
                                       ctypes.sizeof(info)):
        raise OSError(ctypes.get_last_error(), "SetInformationJobObject(memory) failed")
    rate = CPU_RATE(CPU_RATE_ENABLE_HARD_CAP, cpu_percent * 100)
    if not k32.SetInformationJobObject(job, JobObjectCpuRateControlInformation, ctypes.byref(rate),
                                       ctypes.sizeof(rate)):
        raise OSError(ctypes.get_last_error(), "SetInformationJobObject(cpu) failed")
    k32.GetCurrentProcess.restype = wintypes.HANDLE
    if not k32.AssignProcessToJobObject(job, k32.GetCurrentProcess()):
        raise OSError(ctypes.get_last_error(), "AssignProcessToJobObject failed")
    return job


def _posix_limits(mem_bytes):
    import resource
    os.nice(10)
    resource.setrlimit(resource.RLIMIT_AS, (mem_bytes, mem_bytes))


def limit():
    """Caps this process and all its future children. Idempotent; safe to call from every tool."""
    global _JOB
    if _JOB is not None or os.environ.get("ALPHA_GUARD_OFF") == "1":
        return
    mem_bytes, cpu = _settings()
    if os.name == "nt":
        _JOB = _windows_job(mem_bytes, cpu)
    else:
        _posix_limits(mem_bytes)
        _JOB = True
    print(f"resource_guard: capped at {mem_bytes // 1024 ** 3} GB, {cpu}% CPU, low priority, "
          "tree dies with this process", flush=True)


def main(argv):
    if "--" not in argv:
        sys.exit("usage: python tools/resource_guard.py -- <command...>")
    limit()
    return subprocess.run(argv[argv.index("--") + 1:]).returncode


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
