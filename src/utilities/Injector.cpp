//
// Created by Grzegorz on 8/19/2026.
//
#if defined(_WIN32)


#include "utilities/Injector.h"
#include <windows.h>
#include <cstdio>

bool Injector::inject(std::uint32_t pid, const std::filesystem::path& dllPath) const
{


    HANDLE process = OpenProcess(
        PROCESS_VM_OPERATION  |
        PROCESS_VM_WRITE                    |
        PROCESS_VM_READ                     |
        PROCESS_QUERY_INFORMATION           |
        PROCESS_CREATE_THREAD, FALSE, pid);

    if (!process)
    {
        std::fprintf(stderr, "OpenProcess failed: %lu\n", GetLastError());
        return false;
    }

    // VirtualAllocEx a buffer inside the process big enough to hold
    // dllPath as a wide string, including the null terminator.
    const std::wstring& wpath = dllPath.native();
    std::size_t  memorySize = (wpath.size() + 1) * sizeof(wchar_t);
    void* remoteMem  = VirtualAllocEx(process, nullptr, memorySize, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);

    if (!remoteMem)
    {
        std::fprintf(stderr, "VirtualAllocEx failed: %lu\n", GetLastError());
        VirtualFreeEx(process, remoteMem, 0, MEM_RELEASE);
        CloseHandle(process);
        return false;
    }

    // WriteProcessMemory the dllPath bytes into remoteMem.
    size_t bytesWritten = 0;
    WINBOOL writingSucceded = WriteProcessMemory(process, remoteMem, wpath.data(), memorySize, &bytesWritten);
    if (!writingSucceded || bytesWritten != memorySize)
    {
        std::fprintf(stderr, "WriteProcessMemory failed: %lu\n", GetLastError());
        VirtualFreeEx(process, remoteMem, 0, MEM_RELEASE);
        CloseHandle(process);
        return false;
    }

    const HMODULE handleKernel32 = GetModuleHandleW(L"kernel32.dll");
    auto loadLibraryW = reinterpret_cast<LPTHREAD_START_ROUTINE>(GetProcAddress(handleKernel32, "LoadLibraryW"));


    // This starts a thread INSIDE the target that calls LoadLibraryW(remoteMem).
    HANDLE thread = CreateRemoteThread(process, nullptr, 0, loadLibraryW, remoteMem, 0, nullptr);

    if (!thread)
    {
        std::fprintf(stderr, "CreateRemoteThread failed: %lu\n", GetLastError());
        VirtualFreeEx(process, remoteMem, 0, MEM_RELEASE);
        CloseHandle(process);
        return false;
    }

    WaitForSingleObject(thread, INFINITE);
    DWORD exitCode = 0;
    GetExitCodeThread(thread, &exitCode);

    std::printf("LoadLibraryW returned module handle: 0x%lx\n", exitCode);

    CloseHandle(thread);
    VirtualFreeEx(process, remoteMem, 0, MEM_RELEASE);
    CloseHandle(process);
    return exitCode != 0;
}
#endif
