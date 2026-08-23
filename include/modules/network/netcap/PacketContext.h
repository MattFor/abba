//
// Created by Grzegorz on 8/23/2026.
//
#pragma once

#ifndef ABBA_PACKETCONTEXT_H
#define ABBA_PACKETCONTEXT_H
#include <cstdint>
#include <vector>

namespace netcap
{

    enum class PacketDirection : std::uint8_t
    {
        Send = 0,
        Recv = 1
    };

    /**
     * A single socket-level payload captured from the attached process, independent
     * of whatever application protocol the target speaks.
     */
    struct PacketContext
    {
        std::uint32_t              processId{};
        PacketDirection             direction{};
        std::uint64_t               timestamp_ns{};

        /**
         * Raw bytes exactly as passed to / returned from the socket call, (send/recv) TODO: (/WSASend/WSARecv)
         * before any application-level decoding.
         * Capped at hookproto::kMaxPayloadBytes per call.
         */
        std::vector<std::uint8_t> data{};
    };

} // namespace netcap

#endif //ABBA_PACKETCONTEXT_H
