//
// Created by Grzegorz on 9/15/2026.
//

#include "../helpers/CapturedViolations.h"

#include <iostream>
#include <mutex>
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
	ProcessHarness::SpawnedTargetGuard guard( ProcessHarness::kSocketTargetExeName, std::chrono::seconds( 5 ) );
	ProcessHarness::SpawnedTarget      target = guard.target();

	const auto state = std::make_shared<CapturedViolations>();

	NetworkMonitor networkMonitor{};
	networkMonitor.addValidator( std::make_shared<PacketValidator>( PacketValidator::Rules{
	        .min_length      = 2,
	        .max_length      = 3,
	        .reject_all_zero = true,
	        .reject_all_same = true } ) );

	networkMonitor.setInvalidPacketCallback( [state]( const netcap::PacketContext& packet, const std::string& reason )
	                                         {
            {
                std::lock_guard lock(state->mutex);
                state->violations.push_back(Violation{.packet = packet, .reason = reason});
            }
            state->cv.notify_one();
	                                         } );

	if ( !networkMonitor.attach( target.processInfo.dwProcessId ) )
	{
		throw std::runtime_error( "[ERROR] Failed to attach target" );
	}

	ProcessHarness::pressEnter( target.stdInWrite );

	constexpr std::size_t expectedViolationsCount = 2;

	std::unique_lock lock( state->mutex );

	// Frames arrive asynchronously so i wait a bit
	const bool gotExpected = state->cv.wait_for( lock, std::chrono::seconds( 5 ), [&]
	                                             { return state->violations.size() >= expectedViolationsCount; } );
	if ( !gotExpected )
	{
		throw std::runtime_error( "expected violations not captured in time" );
	}

}
} // namespace NetworkTests
