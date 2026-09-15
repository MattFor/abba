//
// Created by Grzegorz on 9/15/2026.
//

#include <algorithm>
#include <condition_variable>
#include <mutex>

#include "../helpers/ProcessHarness.h"
#include "modules/memory/MemoryScanner.h"
#include "modules/network/NetworkMonitor.h"
#include "modules/network/netcap/PacketValidator.h"

namespace IntegrationTests
{
    void network_and_memory_attach_together()
    {
        ProcessHarness::SpawnedTargetGuard guard(ProcessHarness::kSocketTargetExeName);
        ProcessHarness::SpawnedTarget target = guard.target();

        NetworkMonitor networkMonitor{};
        std::mutex resultMutex{};
        std::condition_variable resultcv;
        std::vector<netcap::PacketContext> captured;

        networkMonitor.setInvalidPacketCallback([&](const netcap::PacketContext& packet, const std::string&)
        {
            {
                std::lock_guard lock(resultMutex);
                captured.push_back(packet);
            }
            resultcv.notify_one();
        });
        networkMonitor.addValidator(std::make_shared<PacketValidator>(PacketValidator::Rules{.min_length = 100}));

        if (!networkMonitor.attach(target.processInfo.dwProcessId))
        {
            throw std::runtime_error("Network monitor failed to attach");
        }

        MemoryScanner scanner{};

        if (!scanner.attach(target.processInfo.dwProcessId))
        {
            networkMonitor.detach();
            throw std::runtime_error("Scanner failed to attach");
        }
        const auto snapshotBefore = scanner.snapshot();

        ProcessHarness::pressEnter(target.stdInWrite);

        std::unique_lock lock(resultMutex);
        const bool gotPacket = resultcv.wait_for(lock, std::chrono::milliseconds(3000), [&]
        {
            return !captured.empty();
        });
        lock.unlock();

        if (!gotPacket)
        {
            networkMonitor.detach();
            throw std::runtime_error("Network monitor never captured a packet");
        }

        const auto matches = scanner.scanString("hello from victim_net");
        if (matches.empty())
        {
            networkMonitor.detach();
            throw std::runtime_error("MemoryScanner didn't find marker in memory");
        }

        const std::uintptr_t addr = matches.front();
        std::uint8_t original = 0;
        auto successfulRead = scanner.read(addr,&original,1);
        if (!successfulRead)
        {
            networkMonitor.detach();
            throw std::runtime_error("MemoryScanner failed to read the memory at address");
        }
        const auto poked = static_cast<std::uint8_t>(original^0xFF); // Just what we are writing into that memory

        auto successfulWrite = scanner.write(addr, &poked,1);
        if (!successfulWrite)
        {
            networkMonitor.detach();
            throw std::runtime_error("MemoryScanner failed to write in the memory at address");
        }
        const auto violations = scanner.verify(snapshotBefore);
        const bool sawContentChanged = std::ranges::any_of(violations, [&](const auto& violation)
        {
            return violation.kind == memscan::ViolationKind::ContentChanged;
        });

        auto successfulRewrite = scanner.write(addr, &original,1);
        if (!successfulRewrite)
        {
            networkMonitor.detach();
            throw std::runtime_error("MemoryScanner failed to restore the memory to it's previous form");
        }
        networkMonitor.detach();
        if (!sawContentChanged)
        {
            throw std::runtime_error("verify() missed the write" );
        }
    }
}
