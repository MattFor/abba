//
// Created by Grzegorz on 8/23/2026.
//

#ifndef ABBA_PACKETVALIDATOR_H
#define ABBA_PACKETVALIDATOR_H

#include <optional>

#include "IPacketValidator.h"

/// Windows!
class PacketValidator final : public netcap::IPacketValidator
{
public:
    struct Rules
    {
        std::optional<std::size_t> min_length{};
        std::optional<std::size_t> max_length{};
        bool                       reject_all_zero{
            false
        };
        bool reject_all_same{
            false
        };
    };

    explicit                               PacketValidator(const Rules& rules);
    [[nodiscard]] netcap::ValidationResult validate(const netcap::PacketContext& packet) const override;

private:
    Rules rules_;
};


#endif //ABBA_PACKETVALIDATOR_H
