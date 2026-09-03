//
// Created by Grzegorz on 8/23/2026.
//

#ifndef _WIN32
#error "This file is Windows-only"
#endif

#include <print>
#include <winsock2.h>
#include <windows.h>
#include <tlhelp32.h>

#include "modules/network/netcap/hook/HookProtocol.h"
#include "modules/network/netcap/PacketContext.h"
#include "utilities/Logger.h"


namespace
{
    HANDLE  pipeHandle = INVALID_HANDLE_VALUE;
    SRWLOCK pipeLock    = SRWLOCK_INIT;

    struct SrwExclusiveGuard
    {
        SRWLOCK& lock;

        explicit SrwExclusiveGuard(SRWLOCK& l) : lock(l)
        {
            AcquireSRWLockExclusive(&lock);
        }

        ~SrwExclusiveGuard()
        {
            ReleaseSRWLockExclusive(&lock);
        }

        SrwExclusiveGuard(const SrwExclusiveGuard&)            = delete;
        SrwExclusiveGuard& operator=(const SrwExclusiveGuard&) = delete;
    };


    [[nodiscard]] bool connectPipe()
    {
        std::wstring pipeName = netcap::hookproto::pipeNamePrefix() + std::to_wstring(GetCurrentProcessId());
        WaitNamedPipeW(pipeName.c_str(), 5000);
        pipeHandle = CreateFileW(pipeName.c_str(), GENERIC_WRITE, 0, nullptr, OPEN_EXISTING, 0, nullptr);
        if (pipeHandle == INVALID_HANDLE_VALUE)
        {
            return false;
        }
        return true;
    }

    [[nodiscard]] bool sendFrame(netcap::PacketDirection direction, const std::uint8_t* capturedBytes, size_t capturedBytesLength)
    {
        auto payloadRawByteSize  = static_cast<std::uint32_t>(capturedBytesLength);
        auto payloadBytesToWrite = std::min(payloadRawByteSize, netcap::hookproto::kMaxPayloadBytes);

        netcap::hookproto::FrameHeader header = {
            .process_id = static_cast<std::uint32_t>(GetCurrentProcessId()),
            .direction = static_cast<std::uint8_t>(direction),
            .timestamp_ms = GetTickCount64(),
            .length = payloadBytesToWrite
        };

        SrwExclusiveGuard lock(pipeLock);

        DWORD   headerWrittenBytes    = 0;
        DWORD   headerBytesToWrite    = sizeof( header );
        WINBOOL succededWritingHeader = WriteFile(pipeHandle, &header, sizeof( header ), &headerWrittenBytes, nullptr);
        if (!succededWritingHeader || headerBytesToWrite != headerWrittenBytes)
        {
            p("Something went wrong when writing the network frame header");
            return false;
        }

        if (payloadBytesToWrite == 0)
        {
            return true;
        }

        DWORD   payloadWrittenBytes    = 0;
        WINBOOL succededWritingPayload = WriteFile(pipeHandle, capturedBytes, payloadBytesToWrite, &payloadWrittenBytes, nullptr);

        if (!succededWritingPayload || payloadBytesToWrite != payloadWrittenBytes)
        {
            p("Something went wrong when writing the network frame payload");
            return false;
        }
        return true;
    }

    using SendFn      = int(WSAAPI*)(SOCKET, const char*, int, int);
    using RecvFn      = int(WSAAPI*)(SOCKET, char*, int, int);

    SendFn g_realSend = nullptr;
    RecvFn g_realRecv = nullptr;

    int HookedSend(SOCKET s, const char* buf, int len, int flags)
    {
        auto result = g_realSend(s, buf, len, flags);
        p("[DBG] HookedSend result={}\n", result);
        if (result > 0)
        {
            WINBOOL sendingSucceded = sendFrame(netcap::PacketDirection::Send, reinterpret_cast<const std::uint8_t*>(buf), static_cast<std::size_t>(result));
            p("[DBG] sendFrame ok={}", sendingSucceded);
        }

        return result;
    }

    int HookedRecv(SOCKET s, char* buf, int len, int flags)
    {
        auto result = g_realRecv(s, buf, len, flags);
        p("[DBG] HookedRecv result={}\n", result);

        if (result > 0)
        {
            WINBOOL receivingSucceded = sendFrame(netcap::PacketDirection::Recv, reinterpret_cast<const std::uint8_t*>(buf), static_cast<std::size_t>(result));
            p("[DBG] recvFrame ok={}", receivingSucceded);
        }

        return result;
    }

    bool patchIat(HMODULE module, const char* importDll, const char* importFunc, void* hookFn, void** originalOut)
    {
        const auto addressBase = reinterpret_cast<std::uintptr_t>(module);

        const auto dos = reinterpret_cast<PIMAGE_DOS_HEADER>(addressBase);
        if (dos->e_magic != IMAGE_DOS_SIGNATURE)
        {
            p("DOS signature mismatch. Not a valid PE image.\n");
            return false;
        }

        const auto nt = reinterpret_cast<PIMAGE_NT_HEADERS>(addressBase + dos->e_lfanew);
        if (nt->Signature != IMAGE_NT_SIGNATURE)
        {
            p("NT signature mismatch. Not a valid PE image.\n");
            return false;
        }

        const DWORD addressImportDescriptorsDirectory = nt->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT].VirtualAddress;

        if (addressImportDescriptorsDirectory == 0)
        {
            p("Address descriptors directory imports nothing. Exiting...\n");
            return false;
        }

        const auto descriptorArray = reinterpret_cast<PIMAGE_IMPORT_DESCRIPTOR>(addressBase + addressImportDescriptorsDirectory);

        for (auto importDescriptor = descriptorArray; importDescriptor->Name != 0; importDescriptor++)
        {
            const char* dllName = reinterpret_cast<const char*>(addressBase + importDescriptor->Name);
            // Debuggingmay
            // p("{}:\n", dllName);

            if (stricmp(dllName, importDll) != 0)
            {
                continue;
            }

            auto firstThunk = reinterpret_cast<PIMAGE_THUNK_DATA>(addressBase + importDescriptor->FirstThunk);
            auto thunk      = reinterpret_cast<PIMAGE_THUNK_DATA>(addressBase + ( importDescriptor->OriginalFirstThunk ? importDescriptor->OriginalFirstThunk : importDescriptor->FirstThunk ));

            for (auto thunkIterator = thunk; thunkIterator->u1.AddressOfData != 0; thunkIterator++, firstThunk++)
            {
                if (IMAGE_SNAP_BY_ORDINAL(thunkIterator->u1.Ordinal))
                {
                    continue;
                }

                const auto addressCurrentFunction = reinterpret_cast<PIMAGE_IMPORT_BY_NAME>(addressBase + thunkIterator->u1.AddressOfData);

                if (strcmp(addressCurrentFunction->Name, importFunc) != 0)
                {
                    continue;
                }

                auto* slot          = &firstThunk->u1.Function;
                DWORD oldProtection = 0;

                if (!VirtualProtect(slot, sizeof( *slot ), PAGE_READWRITE, &oldProtection))
                {
                    p("Something went wrong when trying to change right protection of the dll.\n");
                    return false;
                }

                *originalOut = reinterpret_cast<void*>(*slot);
                *slot        = reinterpret_cast<std::uintptr_t>(hookFn);

                if (!VirtualProtect(slot, sizeof( *slot ), oldProtection, &oldProtection))
                {
                    p("Something went wrong when trying to change BACK right protection of the dll. Possibly not critical although unsafe\n");
                }

                return true;
            }
        }

        return false;
    }

    void installHooks()
    {

        HMODULE socketsModule = GetModuleHandleW(L"ws2_32.dll");
        auto    processSendFn = reinterpret_cast<SendFn>(GetProcAddress(socketsModule, "send"));
        g_realSend            = processSendFn;

        auto processRecvFn = reinterpret_cast<RecvFn>(GetProcAddress(socketsModule, "recv"));
        g_realRecv         = processRecvFn;
        if (!g_realRecv || !g_realSend)
        {
            p("Failed to resolve ws2_32 send/recv, aborting hook install\n");
            return;
        }
        HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE | TH32CS_SNAPMODULE32, GetCurrentProcessId());

        if (snapshot == INVALID_HANDLE_VALUE)
        {
            p("Something went wrong when trying to install helper functions.\n");
            return;
        }

        MODULEENTRY32W entry{};
        entry.dwSize = sizeof( entry );

        if (Module32FirstW(snapshot, &entry))
        {
            do
            {
                void* original = nullptr;
                patchIat(entry.hModule, "ws2_32.dll", "send", reinterpret_cast<void*>(&HookedSend), &original);
                patchIat(entry.hModule, "ws2_32.dll", "recv", reinterpret_cast<void*>(&HookedRecv), &original);
            }
            while (Module32NextW(snapshot, &entry));
        }
        CloseHandle(snapshot);
    }

    DWORD WINAPI HookMain(LPVOID)
    {
        if (!connectPipe())
        {
            return -1;
        }
        installHooks();

        const std::wstring eventName = netcap::hookproto::readyEventNamePrefix() + std::to_wstring(GetCurrentProcessId());
        if (HANDLE readyEvent = OpenEventW(EVENT_MODIFY_STATE, FALSE, eventName.c_str()))
        {
            SetEvent(readyEvent);
            CloseHandle(readyEvent);
        }

        return 0;
    }
}


BOOL APIENTRY DllMain(HMODULE module, DWORD reason, LPVOID)
{
    if (reason == DLL_PROCESS_ATTACH)
    {
        DisableThreadLibraryCalls(module);
        CreateThread(nullptr, 0, HookMain, nullptr, 0, nullptr);
        return TRUE;
    }

    return FALSE;
}
