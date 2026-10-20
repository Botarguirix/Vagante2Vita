"""Capture the failing module and offset when an isolated Vita3K process crashes."""
import argparse
import ctypes
import os
import subprocess
import sys
from ctypes import wintypes
from pathlib import Path


class STARTUPINFOW(ctypes.Structure):
    _fields_ = [
        ("cb", wintypes.DWORD), ("lpReserved", wintypes.LPWSTR),
        ("lpDesktop", wintypes.LPWSTR), ("lpTitle", wintypes.LPWSTR),
        ("dwX", wintypes.DWORD), ("dwY", wintypes.DWORD),
        ("dwXSize", wintypes.DWORD), ("dwYSize", wintypes.DWORD),
        ("dwXCountChars", wintypes.DWORD), ("dwYCountChars", wintypes.DWORD),
        ("dwFillAttribute", wintypes.DWORD), ("dwFlags", wintypes.DWORD),
        ("wShowWindow", wintypes.WORD), ("cbReserved2", wintypes.WORD),
        ("lpReserved2", ctypes.POINTER(ctypes.c_ubyte)),
        ("hStdInput", wintypes.HANDLE), ("hStdOutput", wintypes.HANDLE),
        ("hStdError", wintypes.HANDLE),
    ]


class PROCESS_INFORMATION(ctypes.Structure):
    _fields_ = [("hProcess", wintypes.HANDLE), ("hThread", wintypes.HANDLE),
                ("dwProcessId", wintypes.DWORD), ("dwThreadId", wintypes.DWORD)]


class DEBUG_EVENT(ctypes.Structure):
    # Four DWORDs place the union at offset 16 on both supported Windows ABIs.
    _fields_ = [("code", wintypes.DWORD), ("pid", wintypes.DWORD),
                ("tid", wintypes.DWORD), ("alignment", wintypes.DWORD),
                ("payload", ctypes.c_ubyte * 256)]


class MODULEENTRY32W(ctypes.Structure):
    _fields_ = [
        ("dwSize", wintypes.DWORD), ("th32ModuleID", wintypes.DWORD),
        ("th32ProcessID", wintypes.DWORD), ("GlblcntUsage", wintypes.DWORD),
        ("ProccntUsage", wintypes.DWORD), ("modBaseAddr", ctypes.POINTER(ctypes.c_ubyte)),
        ("modBaseSize", wintypes.DWORD), ("hModule", wintypes.HMODULE),
        ("szModule", wintypes.WCHAR * 256), ("szExePath", wintypes.WCHAR * 260),
    ]


kernel32 = ctypes.WinDLL("kernel32", use_last_error=True)
kernel32.CreateProcessW.argtypes = [
    wintypes.LPCWSTR, wintypes.LPWSTR, ctypes.c_void_p, ctypes.c_void_p,
    wintypes.BOOL, wintypes.DWORD, ctypes.c_void_p, wintypes.LPCWSTR,
    ctypes.POINTER(STARTUPINFOW), ctypes.POINTER(PROCESS_INFORMATION),
]
kernel32.CreateProcessW.restype = wintypes.BOOL
kernel32.WaitForDebugEvent.argtypes = [ctypes.POINTER(DEBUG_EVENT), wintypes.DWORD]
kernel32.WaitForDebugEvent.restype = wintypes.BOOL
kernel32.ContinueDebugEvent.argtypes = [wintypes.DWORD, wintypes.DWORD, wintypes.DWORD]
kernel32.ContinueDebugEvent.restype = wintypes.BOOL
kernel32.TerminateProcess.argtypes = [wintypes.HANDLE, wintypes.UINT]
kernel32.CloseHandle.argtypes = [wintypes.HANDLE]
kernel32.CreateToolhelp32Snapshot.argtypes = [wintypes.DWORD, wintypes.DWORD]
kernel32.CreateToolhelp32Snapshot.restype = wintypes.HANDLE
kernel32.Module32FirstW.argtypes = [wintypes.HANDLE, ctypes.POINTER(MODULEENTRY32W)]
kernel32.Module32FirstW.restype = wintypes.BOOL
kernel32.Module32NextW.argtypes = [wintypes.HANDLE, ctypes.POINTER(MODULEENTRY32W)]
kernel32.Module32NextW.restype = wintypes.BOOL
kernel32.OpenThread.argtypes = [wintypes.DWORD, wintypes.BOOL, wintypes.DWORD]
kernel32.OpenThread.restype = wintypes.HANDLE
kernel32.GetThreadContext.argtypes = [wintypes.HANDLE, ctypes.c_void_p]
kernel32.GetThreadContext.restype = wintypes.BOOL
kernel32.ReadProcessMemory.argtypes = [wintypes.HANDLE, ctypes.c_void_p,
                                       ctypes.c_void_p, ctypes.c_size_t,
                                       ctypes.POINTER(ctypes.c_size_t)]
kernel32.ReadProcessMemory.restype = wintypes.BOOL


def modules_for(pid):
    snapshot = kernel32.CreateToolhelp32Snapshot(0x8 | 0x10, pid)
    if snapshot == ctypes.c_void_p(-1).value:
        return []
    result = []
    try:
        item = MODULEENTRY32W()
        item.dwSize = ctypes.sizeof(item)
        ok = kernel32.Module32FirstW(snapshot, ctypes.byref(item))
        while ok:
            result.append((ctypes.addressof(item.modBaseAddr.contents), item.modBaseSize,
                           item.szExePath))
            ok = kernel32.Module32NextW(snapshot, ctypes.byref(item))
    finally:
        kernel32.CloseHandle(snapshot)
    return result


def exception_context(process, tid, modules):
    thread = kernel32.OpenThread(0x8, False, tid)  # THREAD_GET_CONTEXT
    if not thread:
        print(f"thread_context unavailable error={ctypes.get_last_error()}", flush=True)
        return
    try:
        context = ctypes.create_string_buffer(1248)
        ctypes.c_uint32.from_buffer(context, 48).value = 0x0010001F  # CONTEXT_ALL
        if not kernel32.GetThreadContext(thread, context):
            print(f"thread_context failed error={ctypes.get_last_error()}", flush=True)
            return
        rsp = ctypes.c_uint64.from_buffer(context, 152).value
        rbp = ctypes.c_uint64.from_buffer(context, 160).value
        rip = ctypes.c_uint64.from_buffer(context, 248).value
        print(f"context rip=0x{rip:X} rsp=0x{rsp:X} rbp=0x{rbp:X}", flush=True)
        stack = ctypes.create_string_buffer(4096)
        read = ctypes.c_size_t()
        if not kernel32.ReadProcessMemory(process, ctypes.c_void_p(rsp), stack,
                                          len(stack), ctypes.byref(read)):
            print(f"stack_read_failed error={ctypes.get_last_error()}", flush=True)
            return
        candidates = []
        for offset in range(0, read.value - 7, 8):
            value = int.from_bytes(stack.raw[offset:offset + 8], "little")
            mapped = next((m for m in modules if m[0] <= value < m[0] + m[1]), None)
            if mapped:
                candidates.append((offset, value, mapped))
        for offset, value, module in candidates[:32]:
            print(f"stack+0x{offset:X}=0x{value:X} module={module[2]}+0x{value-module[0]:X}", flush=True)
        if not candidates:
            print("stack_return_candidates=none", flush=True)
    finally:
        kernel32.CloseHandle(thread)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("executable", type=Path)
    parser.add_argument("working_directory", type=Path)
    parser.add_argument("config", type=Path)
    parser.add_argument("--frontend", choices=("sdl", "qt"), default="sdl")
    parser.add_argument("--renderer", choices=("Vulkan", "OpenGL"), default="Vulkan")
    parser.add_argument("--skip-title", action="store_true",
                        help="Start the Vita3K frontend without launching an installed title")
    parser.add_argument("--timeout", type=int, default=60)
    args = parser.parse_args()
    exe = args.executable.resolve()
    cwd = args.working_directory.resolve()
    config = args.config.resolve()
    if not exe.is_file() or not config.is_file() or not cwd.is_dir():
        parser.error("executable, config, and working directory must exist")

    command_parts = [str(exe), "--frontend", args.frontend,
        "--load-config", "--keep-config", "--config-location", str(config)]
    if not args.skip_title:
        command_parts += ["-r", "VAGV00006"]
    command = subprocess.list2cmdline(command_parts)
    startup = STARTUPINFOW()
    startup.cb = ctypes.sizeof(startup)
    process = PROCESS_INFORMATION()
    if not kernel32.CreateProcessW(str(exe), ctypes.create_unicode_buffer(command),
            None, None, False, 0x00000002, None, str(cwd), ctypes.byref(startup),
            ctypes.byref(process)):
        raise ctypes.WinError(ctypes.get_last_error())

    print(f"started pid={process.dwProcessId} exe={exe}", flush=True)
    deadline = __import__("time").monotonic() + args.timeout
    first_chance_access_violations = 0
    crash_seen = False
    try:
        while __import__("time").monotonic() < deadline:
            event = DEBUG_EVENT()
            if not kernel32.WaitForDebugEvent(ctypes.byref(event), 1000):
                error = ctypes.get_last_error()
                if error == 121:  # WAIT_TIMEOUT
                    continue
                raise ctypes.WinError(error)
            status = 0x00010002  # DBG_CONTINUE for non-exception events
            if event.code == 1:  # EXCEPTION_DEBUG_EVENT
                raw = ctypes.string_at(ctypes.addressof(event) + 16, 160)
                code, _flags, _record, address, _nparams = __import__("struct").unpack_from("<IIQQI", raw)
                first_chance = __import__("struct").unpack_from("<I", raw, 152)[0]
                if code == 0xC0000005:
                    mapped = next((m for m in modules_for(event.pid) if m[0] <= address < m[0] + m[1]), None)
                    where = (f"module={mapped[2]} base=0x{mapped[0]:X} offset=0x{address-mapped[0]:X}"
                             if mapped else "module=unknown")
                    print(f"access_violation first_chance={first_chance} address=0x{address:X} {where}", flush=True)
                    if first_chance:
                        first_chance_access_violations += 1
                    else:
                        crash_seen = True
                        exception_context(process.hProcess, event.tid, modules_for(event.pid))
                status = 0x00010002 if code == 0x80000003 else 0x80010001
            elif event.code == 5:  # EXIT_PROCESS_DEBUG_EVENT
                exit_code = __import__("struct").unpack_from("<I", bytes(event.payload), 0)[0]
                print(f"process_exit code=0x{exit_code:08X} first_chance_access_violations={first_chance_access_violations}")
                return 0 if exit_code == 0 else 1
            if not kernel32.ContinueDebugEvent(event.pid, event.tid, status):
                raise ctypes.WinError(ctypes.get_last_error())
            if crash_seen:
                continue
        print(f"timeout after {args.timeout}s first_chance_access_violations={first_chance_access_violations}")
        kernel32.TerminateProcess(process.hProcess, 0xC000013A)
        return 124
    finally:
        kernel32.CloseHandle(process.hThread)
        kernel32.CloseHandle(process.hProcess)


if __name__ == "__main__":
    if os.name != "nt":
        raise SystemExit("Windows only")
    raise SystemExit(main())
