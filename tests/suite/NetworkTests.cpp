//
// Created by Grzegorz on 9/15/2026.
//

#include <condition_variable>
#include <iostream>
#include <mutex>
#include <thread>
#include <string>
#include <memory>
#include <stdexcept>
#include <vector>


#include "../helpers/ProcessHarness.h"
#include "modules/network/NetworkMonitor.h"
#include "modules/network/netcap/PacketValidator.h"

namespace NetworkTests
{
    void network_hook_captures_packets()
    {
        ProcessHarness::SpawnedTargetGuard guard{};
        if (!ProcessHarness::spawnTarget(guard.target, ProcessHarness::kSocketTargetExeName))
        {
            throw std::runtime_error("spawn failed");
        }
        p("Spawned abba_socket_target_exe, pid={}", guard.target.processInfo.dwProcessId);

        ProcessHarness::StdoutDrain& drain = guard.drain;
        drain.startDraining(guard.target.stdOutRead);

        if (!drain.waitUntilReady(std::chrono::seconds(5)))
        {
            throw std::runtime_error("target never became ready");
        }

        NetworkMonitor networkMonitor{};
        networkMonitor.addValidator(std::make_shared<PacketValidator>(PacketValidator::Rules{
            .min_length = 2,
            .max_length = 3,
            .reject_all_zero = true,
            .reject_all_same = true
        }));

        std::mutex               resultMutex{};
        std::condition_variable  resultCv;
        std::vector<std::string> violations;

        networkMonitor.setInvalidPacketCallback([&](const netcap::PacketContext& packet, const std::string& reason)
        {
            {
                std::lock_guard lock(resultMutex);
                violations.push_back(reason);
            }
            resultCv.notify_one();
        });
        if (!networkMonitor.attach(guard.target.processInfo.dwProcessId))
        {
            throw std::runtime_error("[ERROR] Failed to attach target");
        }

        ProcessHarness::pressEnter(guard.target.stdInWrite);

        // Frames arrive asynchronously so i wait a bit
        constexpr std::size_t expectedViolationsCount = 2;

        std::unique_lock lock(resultMutex);
        const bool       gotExpected = resultCv.wait_for(lock, std::chrono::seconds(5), [&]
        {
            return violations.size() >= expectedViolationsCount;
        });
        if (!gotExpected)
        {
            throw std::runtime_error("expected violations not captured in time");
        }

        lock.unlock();
        networkMonitor.detach();



    }
}
