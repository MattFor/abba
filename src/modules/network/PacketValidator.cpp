//
// Created by Grzegorz on 8/23/2026.
//

#include "modules/network/netcap/PacketValidator.h"

#include <algorithm>
#include <format>

PacketValidator::PacketValidator(Rules rules) : rules_(rules)
{}

netcap::ValidationResult PacketValidator::validate(const netcap::PacketContext& packet) const
{
    const auto& data = packet.data;
    if (rules_.min_length && data.size() < *rules_.min_length)
    {
        return {
            false,
            "Not enough data in packet"
        };
    }
    if (rules_.max_length && data.size() > *rules_.max_length)
    {
        return {
            false,
            "Too much data in packet"
        };
    }
    if (!data.empty() && ( rules_.reject_all_zero || rules_.reject_all_same ))
    {
        auto front = data.front();
        if (std::ranges::all_of(data.begin(), data.end(), [&front](const auto& x)
        {
            return x == front;
        }))
        {
            if (rules_.reject_all_zero && front == 0)
            {
                return {
                    false,
                    "All data in packet is just zeros"
                };
            }
            if (rules_.reject_all_same)
            {
                return {
                    false,
                    "All data in packet is the same value"
                };
            }
        }
    }
    return {
        true,
        "Packet is valid"
    };
}
