//
// Created by Grzegorz on 8/23/2026.
//
#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include <atomic>
#include <chrono>
#include <filesystem>
#include <mutex>
#include <string>
#include <thread>
#include <condition_variable>

#include "modules/network/NetworkMonitor.h"
#include "modules/network/netcap/PacketValidator.h"
#include "utilities/Logger.h"


namespace
{
    std::filesystem::path socketTargetPath()
    {
        wchar_t buffer[MAX_PATH]{};
        GetModuleFileNameW(nullptr, buffer, MAX_PATH);
        return std::filesystem::path(buffer).parent_path() / L"abba_socket_target.exe";
    }

    struct SpawnedTarget
    {
        PROCESS_INFORMATION processInfo{};
        HANDLE              stdInWrite{
            INVALID_HANDLE_VALUE
        };
        HANDLE stdOutRead{
            INVALID_HANDLE_VALUE
        };
    };

    [[nodiscard]] bool spawnSocketTarget(SpawnedTarget& out)
    {
        SECURITY_ATTRIBUTES sAttr{};
        sAttr.nLength              = sizeof( sAttr );
        sAttr.bInheritHandle       = TRUE;
        sAttr.lpSecurityDescriptor = nullptr;

        HANDLE childStdInRead{
            INVALID_HANDLE_VALUE
        };
        HANDLE childStdInWrite{
            INVALID_HANDLE_VALUE
        };

        if (!CreatePipe(&childStdInRead, &childStdInWrite, &sAttr, 0))
        {
            p("[ERROR] CreatePipe failed: {}", GetLastError());
            return false;
        }


        HANDLE childStdOutRead{
            INVALID_HANDLE_VALUE
        };
        HANDLE childStdOutWrite{
            INVALID_HANDLE_VALUE
        };
        if (!CreatePipe(&childStdOutRead, &childStdOutWrite, &sAttr, 0))
        {
            p("[ERROR] CreatePipe (stdout) failed: {}", GetLastError());
            CloseHandle(childStdInRead);
            CloseHandle(childStdOutWrite);
            return false;
        }
        SetHandleInformation(childStdOutRead, HANDLE_FLAG_INHERIT, 0);

        SetHandleInformation(childStdInWrite, HANDLE_FLAG_INHERIT, 0);

        STARTUPINFOW startupInfo{};
        startupInfo.cb         = sizeof( startupInfo );
        startupInfo.dwFlags    = STARTF_USESTDHANDLES;
        startupInfo.hStdInput  = childStdInRead;
        startupInfo.hStdOutput = childStdOutWrite;
        startupInfo.hStdError  = GetStdHandle(STD_ERROR_HANDLE);

        std::wstring exePath = socketTargetPath().wstring();

        const BOOL created = CreateProcessW(exePath.c_str(), nullptr, // no command-line args needed
                                            nullptr, nullptr, TRUE,   // bInheritHandles - required for the redirect to work
                                            0, nullptr, nullptr, &startupInfo, &out.processInfo);


        CloseHandle(childStdInRead);
        CloseHandle(childStdOutWrite);
        if (!created)
        {
            p("[ERROR] CreateProcessW failed: {}", GetLastError());
            CloseHandle(childStdInWrite);
            CloseHandle(childStdOutRead);
            return false;
        }

        out.stdInWrite = childStdInWrite;
        out.stdOutRead = childStdOutRead;
        return true;
    }

    void pressEnter(HANDLE stdInWrite)
    {
        const char newline = '\n';
        DWORD      written = 0;
        WriteFile(stdInWrite, &newline, 1, &written, nullptr);
    }

    void quitTarget(HANDLE stdInWrite)
    {
        const std::string quit    = "quit\n";
        DWORD             written = 0;
        WriteFile(stdInWrite, quit.data(), static_cast<DWORD>(quit.size()), &written, nullptr);
    }

    struct StdoutDrain
    {
        std::thread       thread;
        std::atomic<bool> ready{
            false
        };
        std::atomic<bool> died{
            false
        };

        void start(HANDLE stdoutRead)
        {
            thread = std::thread([this, stdoutRead]
            {
                std::string buffer;
                char        chunk[256];
                DWORD       bytesRead = 0;

                while (ReadFile(stdoutRead, chunk, sizeof( chunk ), &bytesRead, nullptr) && bytesRead > 0)
                {
                    std::print("{}", std::string_view(chunk, bytesRead)); // keep draining for the whole run, so hook DLL prints (which land in the target's inherited stdout) aren't lost after the ready marker
                    if (!ready)
                    {
                        buffer.append(chunk, bytesRead);
                        if (buffer.find("Press Enter to send a message") != std::string::npos)
                        {
                            ready = true;
                        }
                    }
                }
                died = true;
            });
        }

        bool waitReady(std::chrono::milliseconds timeout)
        {
            const auto deadline = std::chrono::steady_clock::now() + timeout;
            while (!ready && !died && std::chrono::steady_clock::now() < deadline)
            {
                std::this_thread::sleep_for(std::chrono::milliseconds(10));
            }
            return ready.load();
        }

        void join()
        {
            if (thread.joinable())
            {
                thread.join();
            }
        }
    };
}


int main()
{
    SpawnedTarget target{};
    if (!spawnSocketTarget(target))
    {
        return EXIT_FAILURE;
    }
    p("Spawned abba_socket_target_exe, pid={}", target.processInfo.dwProcessId);
    GetModuleFileName(nullptr, nullptr, 0);

    StdoutDrain drain;
    drain.start(target.stdOutRead);

    if (!drain.waitReady(std::chrono::seconds(5)))
    {
        p("[ERROR] Target never became ready");
        quitTarget(target.stdInWrite);
        CloseHandle(target.stdInWrite);
        WaitForSingleObject(target.processInfo.hProcess, 2000);
        drain.join();
        CloseHandle(target.stdOutRead);
        CloseHandle(target.processInfo.hProcess);
        CloseHandle(target.processInfo.hThread);
        return EXIT_FAILURE;
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
    if (!networkMonitor.attach(target.processInfo.dwProcessId))
    {
        p("[ERROR] Failed to attach target: {}", target.processInfo.dwProcessId);
        quitTarget(target.stdInWrite);
        CloseHandle(target.stdInWrite);
        WaitForSingleObject(target.processInfo.hProcess, 2000);
        drain.join();
        CloseHandle(target.stdOutRead);
        CloseHandle(target.processInfo.hProcess);
        CloseHandle(target.processInfo.hThread);
        return EXIT_FAILURE;
    }

    pressEnter(target.stdInWrite);

    // Frames arrive asynchronously so i wait a bit
    constexpr std::size_t expectedCount = 3;

    std::unique_lock lock(resultMutex);
    const bool       gotExpected = resultCv.wait_for(lock, std::chrono::seconds(5), [&]
    {
        return violations.size() >= expectedCount;
    });

    quitTarget(target.stdInWrite);
    WaitForSingleObject(target.processInfo.hProcess, 2000);

    networkMonitor.detach();
    drain.join();

    CloseHandle(target.stdInWrite);
    CloseHandle(target.stdOutRead);
    CloseHandle(target.processInfo.hProcess);
    CloseHandle(target.processInfo.hThread);

    return EXIT_SUCCESS;
}
