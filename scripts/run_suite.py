"""Build, test and run the sorting benchmark reproducibly, then record the environment.

One command that behaves the same on Windows, Linux and WSL:

    python scripts/run_suite.py --profile quick
    python scripts/run_suite.py --profile academic --label my-laptop --report

Steps: configure+build with CMake -> ctest (unit + golden counters) -> collect
environment metadata -> run ./benchmark pinned to one CPU at raised priority ->
write runs/<label>/{results/*.csv, run.log, run_info.json}.

Only the Python standard library is required.
"""

from __future__ import annotations

import argparse
import datetime as dt
import json
import os
import platform
import re
import shutil
import struct
import subprocess
import sys
import time
from pathlib import Path
from typing import Any

PROJECT_ROOT = Path(__file__).resolve().parents[1]
IS_WINDOWS = os.name == "nt"
SCHEMA_VERSION = 1
OS_DISPLAY = {"windows": "Windows", "linux": "Linux", "wsl": "WSL", "macos": "macOS"}

# Same seed and sizes across platforms => counters must match exactly.
PROFILES: dict[str, dict[str, Any]] = {
    "smoke": {"sizes": "100,1000", "repetitions": 5, "dataset": "all", "skip_quadratic_after": 10000},
    "quick": {"sizes": "100,1000,10000", "repetitions": 15, "dataset": "all", "skip_quadratic_after": 10000},
    "academic": {
        "sizes": "100,1000,10000,100000",
        "repetitions": 30,
        "dataset": "all",
        "skip_quadratic_after": 10000,
    },
}


# ---------------------------------------------------------------------------
# helpers
# ---------------------------------------------------------------------------
def run_capture(cmd: list[str], cwd: Path | None = None, timeout: float = 20.0) -> str | None:
    """Run a short command and return stripped stdout, or None on any failure."""
    try:
        proc = subprocess.run(
            cmd, cwd=cwd, capture_output=True, text=True, timeout=timeout, check=False
        )
    except (OSError, subprocess.SubprocessError):
        return None
    if proc.returncode != 0:
        return None
    return proc.stdout.strip()


def log(message: str) -> None:
    print(f"[run_suite] {message}", flush=True)


def read_text(path: str) -> str | None:
    try:
        return Path(path).read_text(encoding="utf-8", errors="replace")
    except OSError:
        return None


# ---------------------------------------------------------------------------
# environment description
# ---------------------------------------------------------------------------
def detect_wsl() -> bool:
    version = read_text("/proc/version") or ""
    return "microsoft" in version.lower()


def os_tag() -> str:
    if IS_WINDOWS:
        return "windows"
    if sys.platform == "darwin":
        return "macos"
    return "wsl" if detect_wsl() else "linux"


def cpu_model() -> str | None:
    if IS_WINDOWS:
        try:
            import winreg

            key = winreg.OpenKey(
                winreg.HKEY_LOCAL_MACHINE, r"HARDWARE\DESCRIPTION\System\CentralProcessor\0"
            )
            value, _ = winreg.QueryValueEx(key, "ProcessorNameString")
            return str(value).strip()
        except OSError:
            return platform.processor() or None
    cpuinfo = read_text("/proc/cpuinfo") or ""
    match = re.search(r"^model name\s*:\s*(.+)$", cpuinfo, re.MULTILINE)
    if match:
        return match.group(1).strip()
    if sys.platform == "darwin":
        return run_capture(["sysctl", "-n", "machdep.cpu.brand_string"])
    return platform.processor() or None


def total_memory_bytes() -> int | None:
    if IS_WINDOWS:
        import ctypes

        class MemoryStatus(ctypes.Structure):
            _fields_ = [
                ("dwLength", ctypes.c_ulong),
                ("dwMemoryLoad", ctypes.c_ulong),
                ("ullTotalPhys", ctypes.c_ulonglong),
                ("ullAvailPhys", ctypes.c_ulonglong),
                ("ullTotalPageFile", ctypes.c_ulonglong),
                ("ullAvailPageFile", ctypes.c_ulonglong),
                ("ullTotalVirtual", ctypes.c_ulonglong),
                ("ullAvailVirtual", ctypes.c_ulonglong),
                ("sullAvailExtendedVirtual", ctypes.c_ulonglong),
            ]

        status = MemoryStatus()
        status.dwLength = ctypes.sizeof(MemoryStatus)
        if ctypes.windll.kernel32.GlobalMemoryStatusEx(ctypes.byref(status)):  # type: ignore[attr-defined]
            return int(status.ullTotalPhys)
        return None
    meminfo = read_text("/proc/meminfo") or ""
    match = re.search(r"^MemTotal:\s+(\d+)\s+kB", meminfo, re.MULTILINE)
    return int(match.group(1)) * 1024 if match else None


def os_description() -> str:
    if IS_WINDOWS:
        return platform.platform()
    pretty = re.search(r'^PRETTY_NAME="?([^"\n]+)"?', read_text("/etc/os-release") or "", re.MULTILINE)
    return pretty.group(1) if pretty else platform.platform()


def power_info() -> dict[str, Any]:
    """Frequency-scaling policy: a big source of run-to-run drift."""
    info: dict[str, Any] = {}
    if IS_WINDOWS:
        scheme = run_capture(["powercfg", "/getactivescheme"])
        if scheme:
            info["windows_power_scheme"] = scheme
    else:
        governor = read_text("/sys/devices/system/cpu/cpu0/cpufreq/scaling_governor")
        if governor:
            info["linux_governor"] = governor.strip()
        elif detect_wsl():
            info["note"] = "WSL2 guest: CPU frequency/governor is controlled by the Windows host"
    return info


def git_info() -> dict[str, Any]:
    commit = run_capture(["git", "rev-parse", "HEAD"], cwd=PROJECT_ROOT)
    if commit is None:
        return {}
    status = run_capture(["git", "status", "--porcelain"], cwd=PROJECT_ROOT) or ""
    return {
        "commit": commit,
        "branch": run_capture(["git", "rev-parse", "--abbrev-ref", "HEAD"], cwd=PROJECT_ROOT),
        "dirty": bool(status),
    }


def cmake_cache_value(build_dir: Path, name: str) -> str | None:
    cache = read_text(str(build_dir / "CMakeCache.txt")) or ""
    match = re.search(rf"^{re.escape(name)}:[A-Z]+=(.*)$", cache, re.MULTILINE)
    return match.group(1).strip() if match else None


def compiler_info(build_dir: Path) -> dict[str, Any]:
    cc = cmake_cache_value(build_dir, "CMAKE_C_COMPILER")
    info: dict[str, Any] = {"path": cc}
    if not cc:
        return info
    banner = run_capture([cc, "--version"])
    info["banner"] = banner.splitlines()[0] if banner else None
    info["target"] = run_capture([cc, "-dumpmachine"])
    info["version"] = run_capture([cc, "-dumpfullversion"]) or run_capture([cc, "-dumpversion"])
    first_line = (info["banner"] or "").lower()
    if "clang" in first_line:
        info["id"] = "clang"
        version_match = re.search(r"version\s+([\d.]+)", first_line)
        if version_match:  # clang's -dumpversion reports the emulated GCC version
            info["version"] = version_match.group(1)
    elif "gcc" in first_line or "g++" in first_line:
        info["id"] = "gcc"
    else:
        info["id"] = Path(cc).stem
    return info


# ---------------------------------------------------------------------------
# CPU pinning / priority
# ---------------------------------------------------------------------------
def windows_cpu_sets() -> list[tuple[int, int, int]] | None:
    """Return (logical_index, core_index, efficiency_class) per logical CPU, or None."""
    import ctypes
    from ctypes import wintypes

    kernel32 = ctypes.WinDLL("kernel32", use_last_error=True)  # type: ignore[attr-defined]
    fn = kernel32.GetSystemCpuSetInformation
    fn.restype = wintypes.BOOL
    fn.argtypes = [ctypes.c_void_p, wintypes.ULONG, ctypes.POINTER(wintypes.ULONG), wintypes.HANDLE, wintypes.ULONG]
    needed = wintypes.ULONG(0)
    fn(None, 0, ctypes.byref(needed), None, 0)
    if needed.value == 0:
        return None
    buffer = ctypes.create_string_buffer(needed.value)
    if not fn(buffer, needed, ctypes.byref(needed), None, 0):
        return None
    raw, offset, cpus = buffer.raw, 0, []
    while offset + 19 <= needed.value:
        size, kind = struct.unpack_from("<II", raw, offset)
        if size == 0:
            break
        if kind == 0:  # CpuSetInformation
            cpus.append((raw[offset + 14], raw[offset + 15], raw[offset + 18]))
        offset += size
    return cpus or None


def choose_pin_cpu(mode: str) -> tuple[int | None, str]:
    """Pick the CPU to pin to. Returns (cpu or None, human reason)."""
    if mode == "none":
        return None, "pinning disabled"
    if mode != "auto":
        return int(mode), "chosen explicitly with --pin"

    if IS_WINDOWS:
        try:
            cpus = windows_cpu_sets()
        except (OSError, AttributeError, struct.error):
            cpus = None
        if cpus:
            best = max(eff for _, _, eff in cpus)
            firsts: dict[int, int] = {}
            for logical, core, eff in sorted(cpus):
                if eff == best and core not in firsts:
                    firsts[core] = logical
            candidates = [logical for _, logical in sorted(firsts.items())]
            pick = next((c for c in candidates if c != 0), candidates[0])
            hybrid = len({eff for _, _, eff in cpus}) > 1
            why = "first thread of a performance core (hybrid CPU: avoids E-cores)" if hybrid else "first thread of a core, skipping CPU 0"
            return pick, why
        return None, "could not read CPU topology; pinning skipped (use --pin N)"

    allowed = sorted(os.sched_getaffinity(0)) if hasattr(os, "sched_getaffinity") else []
    perf = read_text("/sys/devices/cpu_core/cpus")  # native Linux on Intel hybrid CPUs
    if perf and allowed:
        wanted: set[int] = set()
        for part in perf.strip().split(","):
            low, _, high = part.partition("-")
            wanted.update(range(int(low), int(high or low) + 1))
        pool = [c for c in allowed if c in wanted and c != 0] or [c for c in allowed if c in wanted]
        if pool:
            return pool[0], "performance core of a hybrid CPU"
    others = [c for c in allowed if c != 0]
    if others:
        return others[0], "lowest allowed CPU except 0 (topology unknown; use --pin N to override)"
    return None, "no CPU available to pin"


class WindowsAffinity:
    """Set this process' affinity so the benchmark child inherits it; restore on exit."""

    def __init__(self, cpu: int) -> None:
        import ctypes
        from ctypes import wintypes

        self.k32 = ctypes.WinDLL("kernel32", use_last_error=True)  # type: ignore[attr-defined]
        self.k32.GetCurrentProcess.restype = wintypes.HANDLE
        self.k32.SetProcessAffinityMask.argtypes = [wintypes.HANDLE, ctypes.c_size_t]
        self.k32.SetProcessAffinityMask.restype = wintypes.BOOL
        self.k32.GetProcessAffinityMask.argtypes = [
            wintypes.HANDLE,
            ctypes.POINTER(ctypes.c_size_t),
            ctypes.POINTER(ctypes.c_size_t),
        ]
        self.cpu = cpu
        self.process = self.k32.GetCurrentProcess()
        self.previous = ctypes.c_size_t(0)
        system = ctypes.c_size_t(0)
        self.k32.GetProcessAffinityMask(self.process, ctypes.byref(self.previous), ctypes.byref(system))

    def __enter__(self) -> bool:
        return bool(self.k32.SetProcessAffinityMask(self.process, 1 << self.cpu))

    def __exit__(self, *exc: object) -> None:
        if self.previous.value:
            self.k32.SetProcessAffinityMask(self.process, self.previous.value)


# ---------------------------------------------------------------------------
# build
# ---------------------------------------------------------------------------
def pick_generator() -> str | None:
    if shutil.which("ninja"):
        return "Ninja"
    if IS_WINDOWS:
        # CMake refuses "MinGW Makefiles" when sh.exe is on PATH and vice versa.
        return "MSYS Makefiles" if shutil.which("sh") else "MinGW Makefiles"
    return None


def build(args: argparse.Namespace, build_dir: Path) -> None:
    cc = args.cc or os.environ.get("CC") or ("gcc" if shutil.which("gcc") else "clang")
    configure = ["cmake", "-S", str(PROJECT_ROOT), "-B", str(build_dir), f"-DCMAKE_C_COMPILER={cc}"]
    if not (build_dir / "CMakeCache.txt").exists():
        generator = args.generator or pick_generator()
        if generator:
            configure += ["-G", generator]
    log("configure: " + " ".join(configure))
    subprocess.run(configure, check=True)
    log("build")
    subprocess.run(["cmake", "--build", str(build_dir), "--parallel"], check=True)


def run_tests(build_dir: Path) -> None:
    log("ctest (unit tests + golden counters + smoke)")
    subprocess.run(["ctest", "--test-dir", str(build_dir), "--output-on-failure"], check=True)


# ---------------------------------------------------------------------------
# main
# ---------------------------------------------------------------------------
def parse_args() -> argparse.Namespace:
    p = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    p.add_argument("--profile", choices=sorted(PROFILES), default="quick")
    p.add_argument("--seed", type=int, default=42)
    p.add_argument("--repetitions", type=int)
    p.add_argument("--sizes", help="comma-separated sizes (overrides profile)")
    p.add_argument("--dataset", help="random|sorted|reversed|nearly_sorted|few_unique|organ_pipe|sawtooth|zipf|wide_range|all")
    p.add_argument("--skip-quadratic-after", type=int, dest="skip_quadratic_after")
    p.add_argument("--label", help="run directory name under --out (default: auto)")
    p.add_argument("--out", type=Path, default=PROJECT_ROOT / "runs")
    p.add_argument("--build-dir", type=Path)
    p.add_argument("--cc", help="C compiler (default: $CC, then gcc, then clang)")
    p.add_argument("--generator", help="CMake generator (default: Ninja, else a Makefiles flavour)")
    p.add_argument("--pin", default="auto", help="auto (default), none, or a logical CPU index")
    p.add_argument("--no-priority", action="store_true", help="do not raise process priority")
    p.add_argument("--skip-build", action="store_true", help="reuse an existing build directory")
    p.add_argument("--skip-tests", action="store_true", help="do not run ctest before benchmarking")
    p.add_argument("--report", action="store_true", help="build the HTML report for this run afterwards")
    return p.parse_args()


def main() -> int:
    args = parse_args()
    profile = dict(PROFILES[args.profile])
    for key in ("repetitions", "sizes", "dataset", "skip_quadratic_after"):
        value = getattr(args, key)
        if value is not None:
            profile[key] = value

    build_dir = args.build_dir or PROJECT_ROOT / "build" / f"suite-{os_tag()}-{Path(args.cc or 'cc').stem}"
    try:
        if not args.skip_build:
            build(args, build_dir)
        if not args.skip_tests:
            run_tests(build_dir)
    except (subprocess.CalledProcessError, FileNotFoundError) as exc:
        print(f"[run_suite] build/test step failed: {exc}", file=sys.stderr)
        print("[run_suite] refusing to benchmark a broken build.", file=sys.stderr)
        return 2

    binary = build_dir / ("benchmark.exe" if IS_WINDOWS else "benchmark")
    if not binary.exists():
        print(f"[run_suite] benchmark binary not found: {binary}", file=sys.stderr)
        return 2

    compiler = compiler_info(build_dir)
    started = dt.datetime.now().astimezone()
    label = args.label or "-".join(
        [os_tag(), f"{compiler.get('id') or 'cc'}{compiler.get('version') or ''}", started.strftime("%Y%m%d-%H%M")]
    )
    run_dir = args.out / label
    run_dir.mkdir(parents=True, exist_ok=True)

    bench_args = [
        "--seed", str(args.seed),
        "--repetitions", str(profile["repetitions"]),
        "--sizes", str(profile["sizes"]),
        "--dataset", str(profile["dataset"]),
        "--skip-quadratic-after", str(profile["skip_quadratic_after"]),
    ]

    pin_cpu, pin_reason = choose_pin_cpu(args.pin)
    info: dict[str, Any] = {
        "schema_version": SCHEMA_VERSION,
        "label": label,
        "display_name": f"{OS_DISPLAY[os_tag()]} · {compiler.get('id')} {compiler.get('version')}",
        "started_at": started.isoformat(timespec="seconds"),
        "profile": args.profile,
        "benchmark_args": bench_args,
        "cmake": {
            "build_type": cmake_cache_value(build_dir, "CMAKE_BUILD_TYPE"),
            "generator": cmake_cache_value(build_dir, "CMAKE_GENERATOR"),
            "strict": cmake_cache_value(build_dir, "SB_STRICT"),
            "sanitize": cmake_cache_value(build_dir, "SB_SANITIZE"),
        },
        "compiler": compiler,
        "system": {
            "os_tag": os_tag(),
            "os": os_description(),
            "kernel": platform.release(),
            "machine": platform.machine(),
            "is_wsl": detect_wsl(),
            "cpu_model": cpu_model(),
            "logical_cpus": os.cpu_count(),
            "memory_bytes": total_memory_bytes(),
            "power": power_info(),
            "python": platform.python_version(),
        },
        "git": git_info(),
        "pinning": {"mode": args.pin, "cpu": pin_cpu, "reason": pin_reason},
    }

    log(f"run dir: {run_dir}")
    log(f"pinning: cpu={pin_cpu} ({pin_reason})")
    command = [str(binary)] + bench_args
    popen_kwargs: dict[str, Any] = {}
    priority_note = "unchanged"
    if IS_WINDOWS:
        if not args.no_priority:
            popen_kwargs["creationflags"] = subprocess.HIGH_PRIORITY_CLASS  # type: ignore[attr-defined]
            priority_note = "HIGH_PRIORITY_CLASS"
    else:
        def prepare_child() -> None:
            if pin_cpu is not None:
                os.sched_setaffinity(0, {pin_cpu})
            if not args.no_priority:
                try:
                    os.nice(-5)
                except OSError:
                    pass  # needs CAP_SYS_NICE; the run continues at normal priority

        if not args.no_priority:
            priority_note = "nice -5 requested (applied only when permitted)"
        popen_kwargs["preexec_fn"] = prepare_child

    start = time.perf_counter()
    affinity = WindowsAffinity(pin_cpu) if (IS_WINDOWS and pin_cpu is not None) else None
    try:
        if affinity is not None and not affinity.__enter__():
            log("could not set process affinity; continuing unpinned")
            info["pinning"]["cpu"] = None
        with (run_dir / "run.log").open("w", encoding="utf-8", newline="\n") as log_file:
            proc = subprocess.Popen(
                command, cwd=run_dir, stdout=subprocess.PIPE, stderr=subprocess.STDOUT, text=True, **popen_kwargs
            )
            assert proc.stdout is not None
            for line in proc.stdout:
                sys.stdout.write(line)
                log_file.write(line)
            exit_code = proc.wait()
    finally:
        if affinity is not None:
            affinity.__exit__()

    info["priority"] = priority_note
    info["finished_at"] = dt.datetime.now().astimezone().isoformat(timespec="seconds")
    info["duration_seconds"] = round(time.perf_counter() - start, 3)
    info["exit_code"] = exit_code
    (run_dir / "run_info.json").write_text(json.dumps(info, indent=2) + "\n", encoding="utf-8", newline="\n")
    log(f"wrote {run_dir / 'run_info.json'}")

    if exit_code != 0:
        print(f"[run_suite] benchmark exited with code {exit_code}", file=sys.stderr)
        return exit_code

    if args.report:
        report = PROJECT_ROOT / "scripts" / "build_report.py"
        out = run_dir / "report.html"
        subprocess.run([sys.executable, str(report), str(run_dir), "--output", str(out)], check=True)
        log(f"report: {out}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
