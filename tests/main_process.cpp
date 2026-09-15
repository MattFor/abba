//
// Created by Grzegorz on 8/23/2026.
//
#include <iostream>

#include "helpers/ProcessHarness.h"
#include "helpers/TestRunner.h"
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

namespace NetworkTests
{
    void network_hook_captures_packets();
}

int main()
{
    constexpr Test tests_network[] = {
      {
          .name = "NetworkMonitor::hook captures packets",
          .function = NetworkTests::network_hook_captures_packets
      }
    };

    std::size_t total = 0;
    std::size_t passed = 0;
    std::size_t failed = 0;
    std::size_t current = 0;

    if (runTests(tests_network, passed, failed, total, current))
    {
        return EXIT_FAILURE;
    }

	std::print( "\n {}/{} test passed, {} failed\n", passed, total, failed );

    return 0;
}
#endif
