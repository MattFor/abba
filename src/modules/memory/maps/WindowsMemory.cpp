//
// Created by mattfor on 8/16/26.
//

#include "modules/memory/maps/WindowsMemory.h"

#if defined( _WIN32 )

	#define WIN32_LEAN_AND_MEAN

	#include <array>
	#include <string>

	#include <windows.h>
	#include <psapi.h>

	#include "utilities/Logger.h"

namespace
{
[[nodiscard]] std::string narrow( const std::wstring_view text )
{
	if ( text.empty() )
	{
		return {};
	}

	const int required = WideCharToMultiByte( CP_UTF8, 0, text.data(), static_cast<int>( text.size() ), nullptr, 0, nullptr, nullptr );

	if ( required <= 0 )
	{
		return {};
	}

	std::string narrowed( static_cast<std::size_t>( required ), '\0' );
	WideCharToMultiByte( CP_UTF8, 0, text.data(), static_cast<int>( text.size() ), narrowed.data(), required, nullptr, nullptr );

	return narrowed;
}

[[nodiscard]] std::wstring toDosPath( const std::wstring& device )
{
	std::array<wchar_t, MAX_PATH> target{};

	for ( wchar_t letter = L'A'; letter <= L'Z'; ++letter )
	{
		const std::array<wchar_t, 3> drive{
			letter,
			L':',
			L'\0'
		};

		const DWORD length = QueryDosDeviceW( drive.data(), target.data(), static_cast<DWORD>( target.size() ) );

		if ( length == 0 )
		{
			continue;
		}

		const std::wstring_view resolved{
			target.data()
		};

		if ( !resolved.empty() && device.starts_with( resolved ) && device.size() > resolved.size() && device[resolved.size()] == L'\\' )
		{
			return std::wstring{
				drive.data()
			} + device.substr( resolved.size() );
		}
	}

	return device;
}

[[nodiscard]] std::string mappedPath( const HANDLE process, const LPVOID address )
{
	std::array<wchar_t, MAX_PATH> buffer{};

	const DWORD length = GetMappedFileNameW( process, address, buffer.data(), static_cast<DWORD>( buffer.size() ) );

	if ( length == 0 )
	{
		return {};
	}

	return narrow( toDosPath( std::wstring{
	        buffer.data(),
	        length } ) );
}
} // namespace

memmap::Protection WindowsMemory::translateProtection( const std::uint32_t flags ) noexcept
{
	memmap::Protection protection = memmap::Protection::None;

	switch ( flags & 0xFFU )
	{
		case PAGE_READONLY:
			protection = memmap::Protection::Read;
			break;

		case PAGE_READWRITE:
		case PAGE_WRITECOPY:
			protection = memmap::Protection::Read | memmap::Protection::Write;
			break;

		case PAGE_EXECUTE:
			protection = memmap::Protection::Execute;
			break;

		case PAGE_EXECUTE_READ:
			protection = memmap::Protection::Read | memmap::Protection::Execute;
			break;

		case PAGE_EXECUTE_READWRITE:
		case PAGE_EXECUTE_WRITECOPY:
			protection = memmap::Protection::Read | memmap::Protection::Write | memmap::Protection::Execute;
			break;

		case PAGE_NOACCESS:
		default:
			protection = memmap::Protection::None;
			break;
	}

	if ( ( flags & PAGE_GUARD ) != 0 )
	{
		protection |= memmap::Protection::Guard;
	}

	return protection;
}

memmap::RegionType WindowsMemory::translateType( const std::uint32_t flags ) noexcept
{
	switch ( flags )
	{
		case MEM_IMAGE:
			return memmap::RegionType::Image;

		case MEM_MAPPED:
			return memmap::RegionType::Mapped;

		case MEM_PRIVATE:
			return memmap::RegionType::Private;

		default:
			return memmap::RegionType::Unknown;
	}
}

bool WindowsMemory::collect( const std::uint32_t process_id, std::vector<memmap::Region>& regions ) const
{
	HANDLE process = OpenProcess( PROCESS_QUERY_INFORMATION, FALSE, process_id );

	if ( process == nullptr )
	{
		process = OpenProcess( PROCESS_QUERY_LIMITED_INFORMATION, FALSE, process_id );
	}

	if ( process == nullptr )
	{
		p( "[ERROR] MemoryMap could not open process {}", process_id );

		return false;
	}

	SYSTEM_INFO info{};
	GetSystemInfo( &info );

	const auto highest = reinterpret_cast<std::uintptr_t>( info.lpMaximumApplicationAddress );

	std::uintptr_t cursor = reinterpret_cast<std::uintptr_t>( info.lpMinimumApplicationAddress );

	MEMORY_BASIC_INFORMATION block{};

	// Walk through the target process's virtual address space one region at a time
	// VirtualQueryEx fills "block" with information about the region containing the current address
	// The loop stops when we reach the highest address or VirtualQueryEx can no longer return a complete MEMORY_BASIC_INFORMATION.
	while ( cursor <= highest && VirtualQueryEx( process, reinterpret_cast<LPCVOID>( cursor ), &block, sizeof( block ) ) == sizeof( block ) )
	{
		// Where mem region begins
		const auto base = reinterpret_cast<std::uintptr_t>( block.BaseAddress );

		// Immediately after the region
		const auto next = base + block.RegionSize;

		// Only non0 commited regions
		if ( block.State == MEM_COMMIT && block.RegionSize > 0 )
		{
			// Offset informs where region starts relative to allocation
			const auto allocation = reinterpret_cast<std::uintptr_t>( block.AllocationBase );

			// Covnvert into platform independent type
			const auto type = translateType( block.Type );

			// Convert to universal representation
			// P.S MEM_PRIVATE marked as private (duh)
			// [other marked as shared]
			regions.push_back( memmap::Region{
			        .base       = base,
			        .size       = static_cast<std::size_t>( block.RegionSize ),
			        .offset     = allocation != 0 && base >= allocation ? static_cast<std::uint64_t>( base - allocation ) : 0,
			        .protection = translateProtection( block.Protect ) | ( block.Type == MEM_PRIVATE ? memmap::Protection::None : memmap::Protection::Shared ),
			        .type       = type,

			        // Private memory has no mapped path
			        // For mapped mem resolve path before backing
			        .path = type == memmap::RegionType::Private ? std::string{} : mappedPath( process, block.BaseAddress ) } );
		}

		if ( next <= cursor )
		{
			break;
		}

		cursor = next;
	}

	CloseHandle( process );

	return !regions.empty();
}

#endif
