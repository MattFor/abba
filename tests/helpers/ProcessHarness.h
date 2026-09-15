//
// Created by Grzegorz on 9/15/2026.
//

#include <filesystem>
#ifndef _WIN32
#error "This file is Windows-only"
#endif

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <chrono>
#include <atomic>
#include <string>
#include <thread>
#include <string_view>
#include <print>

#include "utilities/Logger.h"


#ifndef ABBA_PROCESSHARNESS_H
#define ABBA_PROCESSHARNESS_H


namespace ProcessHarness
{
    inline constexpr wchar_t kSocketTargetExeName[] = L"abba_socket_target.exe";
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

    class StdoutDrain
    {
        std::thread       thread;
        std::atomic<bool> ready{
            false
        };
        std::atomic<bool> died{
            false
        };
    public:

        bool waitUntilReady(std::chrono::milliseconds timeout) const
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

        void startDraining(HANDLE stdoutRead)
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

    };

    inline std::filesystem::path targetExecutablePath(std::filesystem::path exePath)
    {
        wchar_t buffer[MAX_PATH]{};
        GetModuleFileNameW(nullptr, buffer, MAX_PATH);
        return std::filesystem::path(buffer).parent_path() / exePath;
    }

    [[nodiscard]] inline bool spawnTarget(SpawnedTarget& out, std::filesystem::path exeName)
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

        std::wstring exePath = targetExecutablePath(exeName).wstring();

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

    inline void pressEnter(HANDLE stdInWrite)
    {
        const char newline = '\n';
        DWORD      written = 0;
        WriteFile(stdInWrite, &newline, 1, &written, nullptr);
    }

    inline void quitTarget(HANDLE stdInWrite)
    {
        const std::string preambule        = "quit\n";
        DWORD             preambuleWritten = 0;
        WriteFile(stdInWrite, preambule.data(), static_cast<DWORD>(preambule.size()), &preambuleWritten, nullptr);
    }








    struct SpawnedTargetGuard
    {
        SpawnedTarget target{};
        StdoutDrain   drain;

        ~SpawnedTargetGuard()
        {
            quitTarget(target.stdInWrite);
            WaitForSingleObject(target.processInfo.hProcess, 2000);
            drain.join();
            CloseHandle(target.stdInWrite);
            CloseHandle(target.stdOutRead);
            CloseHandle(target.processInfo.hProcess);
            CloseHandle(target.processInfo.hThread);
        }
    };

} // namespace
#endif //ABBA_PROCESSHARNESS_H
