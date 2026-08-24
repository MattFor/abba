//
// Created by Grzegorz on 8/23/2026.
//

#ifndef ABBA_HOOKPROTOCOL_H
#define ABBA_HOOKPROTOCOL_H

#include <cstdint>

/// Windows!
namespace netcap::hookproto
{
    inline constexpr std::uint32_t kMagic           = 0x50414E43; /// Arbitrary ID
    inline constexpr std::uint32_t kMaxPayloadBytes = 4096;

    inline const wchar_t* pipeNamePrefix()
    {
        return LR"(\\.\pipe\abba_)";
    }

    inline const wchar_t* readyEventNamePrefix()
    {
        return L"abba_hook_ready_";
    }
#pragma pack(push, 1)
    struct FrameHeader
    {
        std::uint32_t magic{
            kMagic
        };
        std::uint32_t process_id{};
        std::uint8_t  direction{};
        std::uint64_t timestamp_ns{};
        std::uint32_t length{};
    };
#pragma pack(pop)
}

#endif //ABBA_HOOKPROTOCOL_H
