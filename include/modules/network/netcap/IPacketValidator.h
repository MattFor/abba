//
// Created by Grzegorz on 8/23/2026.
//

// It's cross platform!

#ifndef ABBA_IPACKETVALIDATOR_H
#define ABBA_IPACKETVALIDATOR_H

#include <string>

#include "PacketContext.h"

/// Windows!
namespace netcap
{
struct ValidationResult
{
	bool valid{
		true
	};

	std::string reason{
		"Packet was correct!"
	};
};

/**
 * A pluggable rule set for judging whether a captured packet is well-formed.
 *
 * NetworkMonitor only knows how to capture raw socket payloads
 * Deciding what "invalid" means for a given target
 * is left to validators supplied by the application (see
 * TemplatePacketValidator for a generic, rule-driven default).
 */
class IPacketValidator
{
  public:
	virtual ~IPacketValidator() = default;

	[[nodiscard]] virtual ValidationResult validate( const PacketContext& packet ) const = 0;
};
} // namespace netcap

#endif // ABBA_IPACKETVALIDATOR_H
