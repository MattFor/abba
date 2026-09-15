//
// Created by Grzegorz on 8/19/2026.
//

#include <memory>
#include <string>
#include <functional>

#include "modules/network/netcap/IPacketValidator.h"
#include "modules/network/netcap/PacketContext.h"
#include "utilities/Logger.h"
#include "modules/network/NetworkMonitor.h"

#if defined( _WIN32 )

	#define WIN32_LEAN_AND_MEAN

	#include <windows.h>
	#include <thread>
	#include <mutex>
	#include <filesystem>
	#include "modules/network/netcap/hook/HookProtocol.h"
	#include "utilities/Injector.h"
	#include <cstdint>

namespace
{
[[nodiscard]] bool overlappedRead( HANDLE pipe, HANDLE event, HANDLE stopEvent, void* buffer, DWORD toRead, DWORD& read )
{
	OVERLAPPED ov{};
	ov.hEvent = event;
	ResetEvent( event );

	if ( ReadFile( pipe, buffer, toRead, nullptr, &ov ) )
	{
		return GetOverlappedResult( pipe, &ov, &read, FALSE );
	}
	if ( GetLastError() != ERROR_IO_PENDING )
	{
		return false;
	}
	HANDLE waitHandles[2] = {
		event,
		stopEvent
	};
	const DWORD waitResult = WaitForMultipleObjects( 2, waitHandles, FALSE, INFINITE );
	// Blocks here until data arrives OR CancelIoEx aborts it (ERROR_OPERATION_ABORTED) - this is the reliable cancel point.
	if ( waitResult == WAIT_OBJECT_0 )
	{
		return GetOverlappedResult( pipe, &ov, &read, FALSE );
	}

	// stopevent fired (or wait failed) - cancel pedning operation, from our own thread
	CancelIoEx( pipe, &ov );
	GetOverlappedResult( pipe, &ov, &read, TRUE );
	return false;
}

std::wstring pipeName( std::uint32_t pid )
{
	return netcap::hookproto::pipeNamePrefix() + std::to_wstring( pid );
}

std::filesystem::path hookDllPath()
{
	wchar_t buffer[MAX_PATH]{};
	GetModuleFileNameW( nullptr, buffer, MAX_PATH );
	return std::filesystem::path( buffer ).parent_path() / L"abba_hook.dll"; // filename TBD, see below
}
} // namespace

struct NetworkMonitor::Impl
{
	HANDLE pipe{
		INVALID_HANDLE_VALUE
	};
	HANDLE ioEvent{
		INVALID_HANDLE_VALUE
	};
	HANDLE stopEvent{
		INVALID_HANDLE_VALUE
	};
	std::thread       serverThread;
	std::atomic<bool> running{
		false
	};
	std::mutex                                             validatorsMutex;
	std::vector<std::shared_ptr<netcap::IPacketValidator>> validators;

	InvalidPacketCallback callback;

	void serverLoop()
	{
		OVERLAPPED connectOv{};
		connectOv.hEvent = ioEvent;
		ResetEvent( ioEvent );

		const BOOL connectedNow = ConnectNamedPipe( pipe, &connectOv );
		bool       connected    = connectedNow;

		if ( !connected )
		{
			DWORD err = GetLastError();
			if ( err == ERROR_PIPE_CONNECTED )
			{
				connected = true;
			}
			else if ( err == ERROR_IO_PENDING )
			{
				DWORD unused = 0;
				connected    = GetOverlappedResult( pipe, &connectOv, &unused, TRUE );
			}
		}
		if ( !connected )
		{
			running = false;
			return;
		}
		p( "[DBG] serverLoop: connected={}", connected );

		std::vector<std::uint8_t> payload;

		while ( running )
		{
			netcap::hookproto::FrameHeader header{};
			DWORD                          read = 0;

			if ( !overlappedRead( pipe, ioEvent, stopEvent, &header, sizeof( header ), read ) || read != sizeof( header ) )
			{
				break;
			}

			if ( header.magic != netcap::hookproto::kMagic || header.length > netcap::hookproto::kMaxPayloadBytes )
			{
				break;
			}

			payload.resize( header.length );

			if ( header.length > 0 )
			{
				DWORD payloadRead = 0;

				if ( !overlappedRead( pipe, ioEvent, stopEvent, payload.data(), header.length, payloadRead ) || payloadRead != header.length )
				{
					break;
				}
			}

			netcap::PacketContext packet{};
			packet.processId    = header.process_id;
			packet.direction    = static_cast<netcap::PacketDirection>( header.direction );
			packet.timestamp_ms = header.timestamp_ms;
			packet.data         = payload;

			dispatch( packet );
		}

		running = false;
	}

	void dispatch( const netcap::PacketContext& packet )
	{
		std::vector<std::shared_ptr<netcap::IPacketValidator>> snapshot;
		{
			std::lock_guard lock( validatorsMutex );
			snapshot = validators;
		}

		for ( const auto& validator : snapshot )
		{
			if ( const netcap::ValidationResult result = validator->validate( packet ); !result.valid && callback )
			{
				callback( packet, result.reason );
			}
		}
	}
};

NetworkMonitor::NetworkMonitor()
    : impl_( std::make_unique<Impl>() )
{
}

NetworkMonitor::~NetworkMonitor()
{
	detach();
};

bool NetworkMonitor::attach( std::uint32_t process_id )
{
	if ( impl_->running )
	{
		return false;
	}

	const std::wstring name = pipeName( process_id );

	impl_->pipe = CreateNamedPipeW( name.c_str(), PIPE_ACCESS_INBOUND | FILE_FLAG_OVERLAPPED, PIPE_TYPE_MESSAGE | PIPE_READMODE_MESSAGE | PIPE_WAIT, 1, 0, 64 * 1024, 0, nullptr );

	if ( impl_->pipe == INVALID_HANDLE_VALUE )
	{
		return false;
	}

	const std::wstring eventName  = netcap::hookproto::readyEventNamePrefix() + std::to_wstring( process_id );
	HANDLE             readyEvent = CreateEventW( nullptr, TRUE /*manual reset*/, FALSE /*initially unset*/, eventName.c_str() );
	impl_->ioEvent                = CreateEventW( nullptr, TRUE, FALSE, nullptr ); // manual reset
	if ( !Injector::inject( process_id, hookDllPath() ) )
	{
		CloseHandle( readyEvent );
		CloseHandle( impl_->pipe );
		impl_->pipe = INVALID_HANDLE_VALUE;
		return false;
	}

	const DWORD waitResult = WaitForSingleObject( readyEvent, 5000 );
	CloseHandle( readyEvent );

	if ( waitResult != WAIT_OBJECT_0 )
	{
		p( "[ERROR] Hook never signaled ready (installHooks timed out or failed)" );
		CloseHandle( impl_->pipe );
		impl_->pipe = INVALID_HANDLE_VALUE;
		return false;
	}

	impl_->running      = true;
	impl_->serverThread = std::thread( [this]
	                                   { impl_->serverLoop(); } );
	return true;
}

void NetworkMonitor::detach()
{
	impl_->running = false;
	CancelIoEx( impl_->pipe, nullptr );
	if ( impl_->serverThread.joinable() )
	{
		impl_->serverThread.join();
	}
	DisconnectNamedPipe( impl_->pipe );
	CloseHandle( impl_->pipe );
	impl_->pipe = INVALID_HANDLE_VALUE;
	CloseHandle( impl_->ioEvent );
	impl_->ioEvent = INVALID_HANDLE_VALUE;
	CloseHandle( impl_->stopEvent );
	impl_->stopEvent = INVALID_HANDLE_VALUE;
}

bool NetworkMonitor::isAttached() const
{
	return impl_->running;
}

void NetworkMonitor::addValidator( std::shared_ptr<netcap::IPacketValidator> validator ) const
{
	std::lock_guard lock( impl_->validatorsMutex );
	impl_->validators.push_back( std::move( validator ) );
}

void NetworkMonitor::setInvalidPacketCallback( InvalidPacketCallback callback )
{
	impl_->callback = std::move( callback );
}

#else // !_WIN32

struct NetworkMonitor::Impl
{
};
NetworkMonitor::NetworkMonitor()
    : impl_( std::make_unique<Impl>() )
{
}
NetworkMonitor::~NetworkMonitor() = default;
bool NetworkMonitor::attach( std::uint32_t )
{
	p( "NetworkMonitor: WinSock IAT hooking is Windows-only; not implemented on this platform." );
	return false;
}
void NetworkMonitor::detach()
{
}
bool NetworkMonitor::isAttached() const
{
	return false;
}
void NetworkMonitor::addValidator( std::shared_ptr<netcap::IPacketValidator> )
{
}
void NetworkMonitor::setInvalidPacketCallback( InvalidPacketCallback )
{
}

#endif
