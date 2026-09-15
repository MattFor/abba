//
// Created by Grzegorz on 8/23/2026.
//
#include <iostream>

#include "helpers/ProcessHarness.h"
#ifndef _WIN32
	#error "Injector.cpp is Windows-only"
#endif

#if defined( _WIN32 )
	#define WIN32_LEAN_AND_MEAN
	#include <windows.h>

	#include <mutex>
	#include <atomic>
	#include <chrono>
	#include <string>
	#include <thread>
	#include <filesystem>
	#include <condition_variable>

	#include "utilities/Logger.h"
	#include "modules/network/NetworkMonitor.h"
	#include "modules/network/netcap/PacketValidator.h"


int main()
{
    ProcessHarness::SpawnedTarget target{};
	if ( !spawnSocketTarget( target ) )
	{
		return EXIT_FAILURE;
	}
	p( "Spawned abba_socket_target_exe, pid={}", target.processInfo.dwProcessId );
	GetModuleFileName( nullptr, nullptr, 0 );

    ProcessHarness::StdoutDrain drain;
    drain.startDraining( target.stdOutRead );

	if ( !drain.waitUntilReady( std::chrono::seconds( 5 ) ) )
	{
		p( "[ERROR] Target never became ready" );
        ProcessHarness::quitTarget( target.stdInWrite );
		CloseHandle( target.stdInWrite );
		WaitForSingleObject( target.processInfo.hProcess, 2000 );
		drain.join();
		CloseHandle( target.stdOutRead );
		CloseHandle( target.processInfo.hProcess );
		CloseHandle( target.processInfo.hThread );
		return EXIT_FAILURE;
	}

	NetworkMonitor networkMonitor{};
	networkMonitor.addValidator( std::make_shared<PacketValidator>( PacketValidator::Rules{
	        .min_length      = 2,
	        .max_length      = 3,
	        .reject_all_zero = true,
	        .reject_all_same = true } ) );

	std::mutex               resultMutex{};
	std::condition_variable  resultCv;
	std::vector<std::string> violations;

	networkMonitor.setInvalidPacketCallback( [&]( const netcap::PacketContext& packet, const std::string& reason )
	                                         {
        {
            std::lock_guard lock(resultMutex);
            violations.push_back(reason);
        }
        resultCv.notify_one(); } );
	if ( !networkMonitor.attach( target.processInfo.dwProcessId ) )
	{
		p( "[ERROR] Failed to attach target: {}", target.processInfo.dwProcessId );
        ProcessHarness::quitTarget( target.stdInWrite );
		CloseHandle( target.stdInWrite );
		WaitForSingleObject( target.processInfo.hProcess, 2000 );
		drain.join();
		CloseHandle( target.stdOutRead );
		CloseHandle( target.processInfo.hProcess );
		CloseHandle( target.processInfo.hThread );
		return EXIT_FAILURE;
	}

    ProcessHarness::pressEnter( target.stdInWrite );

	// Frames arrive asynchronously so i wait a bit
	constexpr std::size_t expectedCount = 3;

	std::unique_lock lock( resultMutex );
	const bool       gotExpected = resultCv.wait_for( lock, std::chrono::seconds( 5 ), [&]
	                                                  { return violations.size() >= expectedCount; } );
	lock.unlock();
	// Two threads are writting to the stdout here so that's why it bugs out, i am too lazy to fix it for now
	p( "Scripted message done. Now you can type your own, 'quit' to stop." );

	std::string line;
	while ( std::getline( std::cin, line ) )
	{
		std::string toSend  = line + "\n";
		DWORD       written = 0;

		WriteFile( target.stdInWrite, toSend.data(), toSend.size(), &written, nullptr );

		if ( line == "quit" )
		{
			break;
		}
	}

	WaitForSingleObject( target.processInfo.hProcess, 2000 );
	p( "[DBG] before detach" );
	networkMonitor.detach();
	p( "[DBG] after detach" );
	drain.join();
	p( "[DBG] after drain join" );

	CloseHandle( target.stdInWrite );
	CloseHandle( target.stdOutRead );
	CloseHandle( target.processInfo.hProcess );
	CloseHandle( target.processInfo.hThread );

	return EXIT_SUCCESS;
}
#endif
