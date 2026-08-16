//
// Created by mattfor on 8/16/26.
//

#if defined(_WIN32)
#include <windows.h>
#elif defined(__linux__)
#include <sys/uio.h>
#else
#error "Unsupported platform"
#endif

#include "modules/memory/MemoryScanner.h"

bool MemoryScanner::attach(const std::uint32_t process_id)
{
#if defined(__linux__)
    this->pid = process_id;
    return true;

#elif defined(_WIN32)
    this->pid = process_id; return true;

#else
    return false;
#endif
}

bool MemoryScanner::read(const std::uintptr_t address, void* buffer, const std::size_t size) const
{
#if defined(__linux__)
    const iovec local{
        .iov_base = buffer,
        .iov_len = size
    };

    const iovec remote{
        .iov_base = reinterpret_cast<void*>(address),
        .iov_len = size
    };

    const auto result = process_vm_readv(static_cast<pid_t>(this->pid), &local, 1, &remote, 1, 0);

    return result == static_cast<ssize_t>(size);

#elif defined(_WIN32)
    HANDLE process = OpenProcess(PROCESS_VM_READ | PROCESS_QUERY_INFORMATION, FALSE, this->pid); if (process == nullptr)
    {
        return false;
    } SIZE_T bytesRead{}; const BOOL success = ReadProcessMemory(process, reinterpret_cast<const void*>(address), buffer, size, &bytesRead); CloseHandle(process); return success && bytesRead == size;

#else
    return false;
#endif
}
