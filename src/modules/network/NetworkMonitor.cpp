//
// Created by Grzegorz on 8/19/2026.
//

#include <cstdint> // NOTE: unused on linux
#include <memory>
#include <string>
#include <functional>

#include "modules/network/netcap/IPacketValidator.h"
#include "modules/network/netcap/PacketContext.h"
#include "utilities/Logger.h"
#include "modules/network/NetworkMonitor.h"


#if defined(_WIN32)


#define WIN32_LEAN_AND_MEAN

#include <windows.h>
#include <thread>
#include <mutex>
#include <filesystem>
#include "modules/network/netcap/hook/HookProtocol.h"
#include "utilities/Injector.h"

namespace
{
    std::wstring pipeName(std::uint32_t pid)
    {
        return netcap::hookproto::pipeNamePrefix() + std::to_wstring(pid);
    }

    std::filesystem::path hookDllPath()
    {
        wchar_t buffer[MAX_PATH]{};
        GetModuleFileNameW(nullptr, buffer, MAX_PATH);
        return std::filesystem::path(buffer).parent_path() / L"abba_hook.dll"; // filename TBD, see below
    }
}struct NetworkMonitor::Impl
{
    HANDLE pipe{
        INVALID_HANDLE_VALUE
    };
    std::thread       serverThread;
    std::atomic<bool> running{
        false
    };
    std::mutex                                             validatorsMutex;
    std::vector<std::shared_ptr<netcap::IPacketValidator>> validators;

    InvalidPacketCallback callback;

    void serverLoop()
    {
        const BOOL connectedNow = ConnectNamedPipe(pipe, nullptr);
        const bool connected    = connectedNow || GetLastError() == ERROR_PIPE_CONNECTED;
        p("[DBG] serverLoop: connected={}", connected);

        if (!connected)
        {
            running = false;
            return;
        }

        std::vector<std::uint8_t> payload;

        while (running)
        {
            netcap::hookproto::FrameHeader header{};
            DWORD                          read = 0;

            if (!ReadFile(pipe, &header, sizeof( header ), &read, nullptr) || read != sizeof( header ))
            {
                break;
            }

            if (header.magic != netcap::hookproto::kMagic || header.length > netcap::hookproto::kMaxPayloadBytes)
            {
                break;
            }

            payload.resize(header.length);

            if (header.length > 0)
            {
                DWORD payloadRead = 0;

                if (!ReadFile(pipe, payload.data(), header.length, &payloadRead, nullptr) || payloadRead != header.length)
                {
                    break;
                }
            }

            netcap::PacketContext packet{};
            packet.processId    = header.process_id;
            packet.direction    = static_cast<netcap::PacketDirection>(header.direction);
            packet.timestamp_ns = header.timestamp_ns;
            packet.data         = payload;

            dispatch(packet);
        }

        running = false;
    }

    void dispatch(const netcap::PacketContext& packet)
    {
        std::lock_guard lock(validatorsMutex);

        for (const auto& validator : validators)
        {
            if (const netcap::ValidationResult result = validator->validate(packet); !result.valid && callback)
            {
                callback(packet, result.reason);
            }
        }
    }
};NetworkMonitor::NetworkMonitor()
    : impl_(std::make_unique<Impl>())
{}NetworkMonitor::~NetworkMonitor()
{
    detach();
};bool NetworkMonitor::attach(std::uint32_t process_id)
{
    if (impl_->running)
    {
        return false;
    }

    const std::wstring name = pipeName(process_id);

    impl_->pipe = CreateNamedPipeW(name.c_str(), PIPE_ACCESS_INBOUND, PIPE_TYPE_MESSAGE | PIPE_READMODE_MESSAGE | PIPE_WAIT, 1, 0, 64 * 1024, 0, nullptr);

    if (impl_->pipe == INVALID_HANDLE_VALUE)
    {
        return false;
    }

    const std::wstring eventName  = netcap::hookproto::readyEventNamePrefix() + std::to_wstring(process_id);
    HANDLE             readyEvent = CreateEventW(nullptr, TRUE /*manual reset*/, FALSE /*initially unset*/, eventName.c_str());

    if (!Injector{}.inject(process_id, hookDllPath()))
    {
        CloseHandle(readyEvent);
        CloseHandle(impl_->pipe);
        impl_->pipe = INVALID_HANDLE_VALUE;
        return false;
    }


    const DWORD waitResult = WaitForSingleObject(readyEvent, 5000);
    CloseHandle(readyEvent);

    if (waitResult != WAIT_OBJECT_0)
    {
        p("[ERROR] Hook never signaled ready (installHooks timed out or failed)");
        CloseHandle(impl_->pipe);
        impl_->pipe = INVALID_HANDLE_VALUE;
        return false;
    }

    impl_->running      = true;
    impl_->serverThread = std::thread([this]
    {
        impl_->serverLoop();
    });
    return true;
}void NetworkMonitor::detach()
{
    impl_->running = false;
    CancelIoEx(impl_->pipe, nullptr);
    DisconnectNamedPipe(impl_->pipe);
    if (impl_->serverThread.joinable())
    {
        impl_->serverThread.join();
    }
    CloseHandle(impl_->pipe);
    impl_->pipe = INVALID_HANDLE_VALUE;
}bool NetworkMonitor::isAttached() const
{
    return impl_->running;
}void NetworkMonitor::addValidator(std::shared_ptr<netcap::IPacketValidator> validator) const
{
    std::lock_guard lock(impl_->validatorsMutex);
    impl_->validators.push_back(std::move(validator));
}void NetworkMonitor::setInvalidPacketCallback(InvalidPacketCallback callback)
{
    impl_->callback = std::move(callback);
}

#else // !_WIN32

struct NetworkMonitor::Impl
{};

NetworkMonitor::NetworkMonitor()
    : impl_(std::make_unique<Impl>())
{}

NetworkMonitor::~NetworkMonitor() = default;

bool NetworkMonitor::attach(std::uint32_t)
{
    p("NetworkMonitor: WinSock IAT hooking is Windows-only; not implemented on this platform.");
    return false;
}

void NetworkMonitor::detach()
{}

bool NetworkMonitor::isAttached() const
{
    return false;
}

void NetworkMonitor::addValidator(std::shared_ptr<netcap::IPacketValidator>)
{}

void NetworkMonitor::setInvalidPacketCallback(InvalidPacketCallback)
{}

#endif
