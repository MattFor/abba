//
// Created by Grzegorz on 10/22/2026.
//

#ifndef ABBA_CAPTUREDVIOLATIONS_H
#define ABBA_CAPTUREDVIOLATIONS_H
#include "modules/network/netcap/PacketContext.h"
#include "modules/network/netcap/hook/HookProtocol.h"

#include <condition_variable>
#include <vector>
#include <mutex>
#include <condition_variable>
#include <string>

struct Violation
{
	netcap::PacketContext packet;
	std::string reason;
};

struct CapturedViolations
{
	std::mutex mutex;
	std::condition_variable cv;
	std::vector<Violation> violations;
};


#endif // ABBA_CAPTUREDVIOLATIONS_H
