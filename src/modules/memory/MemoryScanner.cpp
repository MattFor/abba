//
// Created by mattfor on 8/16/26.
//

#include "modules/memory/MemoryScanner.h"

#include <format>
#include <utility>
#include <algorithm>

#if defined( _WIN32 )

	#define WIN32_LEAN_AND_MEAN

	#include <windows.h>

#elif defined( __linux__ )

	#include <cerrno>
	#include <csignal>
	#include <sys/uio.h>
	#include <filesystem>

#else
	#error "Unsupported platform"
#endif

#include "utilities/Hashing.h"
#include "utilities/Logger.h"

namespace
{
[[nodiscard]] std::optional<std::uint8_t> hexDigit( const char character ) noexcept
{
	if ( character >= '0' && character <= '9' )
	{
		return static_cast<std::uint8_t>( character - '0' );
	}

	if ( character >= 'a' && character <= 'f' )
	{
		return static_cast<std::uint8_t>( character - 'a' + 10 );
	}

	if ( character >= 'A' && character <= 'F' )
	{
		return static_cast<std::uint8_t>( character - 'A' + 10 );
	}

	return std::nullopt;
}

[[nodiscard]] bool isWildcard( const char character ) noexcept
{
	return character == '?' || character == '*';
}

void collectMatches( const std::uint8_t* data, const std::size_t size, const memscan::Pattern& pattern, const std::uintptr_t base, const std::size_t alignment, const std::size_t limit, std::vector<std::uintptr_t>& found )
{
	const std::size_t need = pattern.size();

	// No match if buffer's smaller than pattern
	if ( size < need )
	{
		return;
	}

	std::size_t lead = 0;

	// Find first byte in pattern that has AT LEAST one matching bit
	// Then use it as anchor for faster searching
	while ( lead < need && pattern.mask[lead] == 0 )
	{
		++lead;
	}

	// Pattern contains wildcards only, no useful anchor byte
	if ( lead == need )
	{
		return;
	}

	const std::uint8_t anchor       = pattern.bytes[lead];
	const bool         exact_anchor = pattern.mask[lead] == 0xFFU;
	const std::size_t  last         = size - need;

	std::size_t index = 0;

	while ( index <= last )
	{
		// If anchor must be exact use memchr to quickly find possible matches instead of checking manually
		if ( exact_anchor )
		{
			const auto* hit = static_cast<const std::uint8_t*>( std::memchr( data + index + lead, anchor, last - index + 1 ) );

			// No more bytes == no more anchors
			if ( hit == nullptr )
			{
				return;
			}

			// Convert anchor pos back to start of pattern
			index = static_cast<std::size_t>( hit - data ) - lead;
		}

		// Check full pattern
		if ( pattern.matches( data + index ) )
		{
			// Only keep matches where the address satisfies alignment
			if ( const std::uintptr_t address = base + index; alignment <= 1 || address % alignment == 0 )
			{
				found.push_back( address );

				// Enough matches found, exit time!
				if ( limit != 0 && found.size() >= limit )
				{
					return;
				}
			}
		}

		++index;
	}
}
} // namespace

std::optional<memscan::Pattern> memscan::Pattern::parse( const std::string_view signature )
{
	Pattern pattern{};

	std::size_t index = 0;

	while ( index < signature.size() )
	{
		// Skip byte separators
		if ( const char character = signature[index]; character == ' ' || character == '\t' || character == ',' )
		{
			++index;
			continue;
		}

		std::size_t stop = index;

		while ( stop < signature.size() && signature[stop] != ' ' && signature[stop] != '\t' && signature[stop] != ',' )
		{
			++stop;
		}

		const std::string_view token = signature.substr( index, stop - index );
		index                        = stop;

		// Token must have AT MOST 2 wildcards
		if ( token.size() > 2 )
		{
			return std::nullopt;
		}

		std::uint8_t value = 0;
		std::uint8_t mask  = 0;

		// Parse each hex digit + build byte value + mask
		for ( const char digit : token )
		{
			value = static_cast<std::uint8_t>( value << 4U );
			mask  = static_cast<std::uint8_t>( mask << 4U );

			if ( isWildcard( digit ) )
			{
				continue;
			}

			const auto parsed = hexDigit( digit );

			// Rejectuib for anything ese other than hex digit/wildcard
			if ( !parsed.has_value() )
			{
				return std::nullopt;
			}

			value = static_cast<std::uint8_t>( value | *parsed );
			mask  = static_cast<std::uint8_t>( mask | 0x0FU );
		}

		// Single wildcard represents unknown byte
		if ( token.size() == 1 )
		{
			mask = static_cast<std::uint8_t>( mask | ( isWildcard( token[0] ) ? 0x00U : 0xF0U ) );
		}

		pattern.bytes.push_back( static_cast<std::uint8_t>( value & mask ) );
		pattern.mask.push_back( mask );
	}

	if ( !pattern.valid() )
	{
		return std::nullopt;
	}

	return pattern;
}

memscan::Pattern memscan::Pattern::exact( const std::span<const std::uint8_t> data )
{
	Pattern pattern{};

	// Copy bytes into patern (no changes)
	pattern.bytes.assign( data.begin(), data.end() );

	// Set every bit mask so every bit must be an exact match
	pattern.mask.assign( data.size(), 0xFFU );

	return pattern;
}

bool memscan::Pattern::valid() const noexcept
{
	// Valid pattern must have each byte with a corresponding mask entry
	if ( this->bytes.empty() || this->bytes.size() != this->mask.size() )
	{
		return false;
	}

	// No wildcard only patterns
	return std::ranges::any_of( this->mask, []( const std::uint8_t entry )
	                            { return entry != 0; } );
}

bool memscan::Pattern::matches( const std::uint8_t* data ) const noexcept
{
	// Check each byte against mask
	for ( std::size_t index = 0; index < this->bytes.size(); ++index )
	{
		if ( ( data[index] & this->mask[index] ) != this->bytes[index] )
		{
			return false;
		}
	}

	return true;
}

std::string_view memscan::toString( const ViolationKind kind ) noexcept
{
	switch ( kind )
	{
		case ViolationKind::Missing:
			return "missing";

		case ViolationKind::Resized:
			return "resized";

		case ViolationKind::ProtectionChanged:
			return "protection changed";

		case ViolationKind::ContentChanged:
			return "content changed";

		case ViolationKind::Unreadable:
			return "unreadable";

		default:
			return "unknown";
	}
}

MemoryScanner::MemoryScanner() = default;

MemoryScanner::~MemoryScanner()
{
	this->detach();
}

// Get ownership of other scanners' resources, clean it
MemoryScanner::MemoryScanner( MemoryScanner&& other ) noexcept
    : pid( std::exchange( other.pid, 0 ) ),
      handle( std::exchange( other.handle, nullptr ) ),
      writable( std::exchange( other.writable, false ) ),
      layout( std::move( other.layout ) )
{
}

MemoryScanner& MemoryScanner::operator=( MemoryScanner&& other ) noexcept
{
	if ( this != &other )
	{
		// Release resources owned byt THIS scanner before taking ownership of OTHER scanner's resources
		this->detach();

		this->pid      = std::exchange( other.pid, 0 );
		this->handle   = std::exchange( other.handle, nullptr );
		this->writable = std::exchange( other.writable, false );
		this->layout   = std::move( other.layout );
	}

	return *this;
}

bool MemoryScanner::attach( const std::uint32_t process_id )
{
	this->detach();

	// PID 0 invalid target
	if ( process_id == 0 )
	{
		return false;
	}

#if defined( __linux__ )
	std::error_code error{};

	if ( !std::filesystem::exists( "/proc/" + std::to_string( process_id ), error ) || error )
	{
		p( "[ERROR] MemoryScanner could not attach to process {}", process_id );

		return false;
	}

	this->pid      = process_id;
	this->writable = true;

	return true;

#elif defined( _WIN32 )
	// Request handlew with rw mem access
	HANDLE process = OpenProcess( PROCESS_VM_READ | PROCESS_VM_WRITE | PROCESS_VM_OPERATION | PROCESS_QUERY_INFORMATION, FALSE, process_id );

	const bool granted_write = process != nullptr;

	// If full perms not given, try read only
	if ( process == nullptr )
	{
		process = OpenProcess( PROCESS_VM_READ | PROCESS_QUERY_INFORMATION, FALSE, process_id );
	}

	if ( process == nullptr )
	{
		p( "[ERROR] MemoryScanner could not attach to process {}", process_id );

		return false;
	}

	this->pid      = process_id;
	this->handle   = process;
	this->writable = granted_write;

	return true;

#else
	return false;
#endif
}

void MemoryScanner::detach() noexcept
{
#if defined( _WIN32 )
	if ( this->handle != nullptr )
	{
		CloseHandle( static_cast<HANDLE>( this->handle ) );
	}
#endif

	this->pid      = 0;
	this->handle   = nullptr;
	this->writable = false;

	if ( this->layout )
	{
		this->layout->clear();
	}
}

bool MemoryScanner::isAttached() const noexcept
{
#if defined( _WIN32 )
	return this->pid != 0 && this->handle != nullptr;
#else
	return this->pid != 0;
#endif
}

bool MemoryScanner::isWritable() const noexcept
{
	return this->isAttached() && this->writable;
}

bool MemoryScanner::isRunning() const noexcept
{
	if ( !this->isAttached() )
	{
		return false;
	}

#if defined( __linux__ )
	return kill( static_cast<pid_t>( this->pid ), 0 ) == 0 || errno == EPERM;

#elif defined( _WIN32 )
	DWORD status = 0;

	if ( GetExitCodeProcess( static_cast<HANDLE>( this->handle ), &status ) == FALSE )
	{
		return false;
	}

	return status == STILL_ACTIVE;

#else
	return false;
#endif
}

std::uint32_t MemoryScanner::processId() const noexcept
{
	return this->pid;
}

std::size_t MemoryScanner::readPartial( const std::uintptr_t address, void* buffer, const std::size_t size ) const
{
	if ( !this->isAttached() || buffer == nullptr || size == 0 )
	{
		return 0;
	}

#if defined( __linux__ )
	const iovec local{
		.iov_base = buffer,
		.iov_len  = size
	};

	const iovec remote{
		.iov_base = reinterpret_cast<void*>( address ),
		.iov_len  = size
	};

	const ssize_t result = process_vm_readv( static_cast<pid_t>( this->pid ), &local, 1, &remote, 1, 0 );

	return result > 0 ? static_cast<std::size_t>( result ) : 0;

#elif defined( _WIN32 )
	SIZE_T transferred = 0;

	// To chyba ma tak być ale szczerze nwm copied from the internet
	ReadProcessMemory( static_cast<HANDLE>( this->handle ), reinterpret_cast<LPCVOID>( address ), buffer, size, &transferred );

	return static_cast<std::size_t>( transferred );

#else
	return 0;
#endif
}

bool MemoryScanner::read( const std::uintptr_t address, void* buffer, const std::size_t size ) const
{
	if ( !this->isAttached() )
	{
		return false;
	}

	if ( size == 0 )
	{
		return true;
	}

	if ( buffer == nullptr )
	{
		return false;
	}

	return this->readPartial( address, buffer, size ) == size;
}

bool MemoryScanner::write( const std::uintptr_t address, const void* buffer, const std::size_t size ) const
{
	if ( !this->isAttached() )
	{
		return false;
	}

	if ( size == 0 )
	{
		return true;
	}

	if ( buffer == nullptr )
	{
		return false;
	}

#if defined( __linux__ )
	const iovec local{
		.iov_base = const_cast<void*>( buffer ),
		.iov_len  = size
	};

	const iovec remote{
		.iov_base = reinterpret_cast<void*>( address ),
		.iov_len  = size
	};

	const ssize_t result = process_vm_writev( static_cast<pid_t>( this->pid ), &local, 1, &remote, 1, 0 );

	return result == static_cast<ssize_t>( size );

#elif defined( _WIN32 )
	SIZE_T transferred = 0;

	const BOOL success = WriteProcessMemory( static_cast<HANDLE>( this->handle ), reinterpret_cast<LPVOID>( address ), buffer, size, &transferred );

	return success != FALSE && transferred == size;

#else
	return false;
#endif
}

std::optional<std::vector<std::uint8_t>> MemoryScanner::readBytes( const std::uintptr_t address, const std::size_t size ) const
{
	std::vector<std::uint8_t> data( size );

	if ( !this->read( address, data.data(), size ) )
	{
		return std::nullopt;
	}

	return data;
}

std::optional<std::string> MemoryScanner::readString( const std::uintptr_t address, const std::size_t max_length ) const
{
	if ( max_length == 0 )
	{
		return std::string{};
	}

	std::string text( max_length, '\0' );

	const std::size_t transferred = this->readPartial( address, text.data(), max_length );

	if ( transferred == 0 )
	{
		return std::nullopt;
	}

	text.resize( transferred );

	if ( const auto terminator = text.find( '\0' ); terminator != std::string::npos )
	{
		text.resize( terminator );
	}

	return text;
}

bool MemoryScanner::refreshMap() const
{
	if ( !this->isAttached() )
	{
		return false;
	}

	if ( !this->layout )
	{
		this->layout = MemoryMap::create();
	}

	if ( !this->layout )
	{
		return false;
	}

	return this->layout->refresh( this->pid );
}

const MemoryMap* MemoryScanner::map() const noexcept
{
	return this->layout.get();
}

const MemoryMap* MemoryScanner::ensureMap() const
{
	if ( !this->isAttached() )
	{
		return nullptr;
	}

	if ( this->layout && !this->layout->empty() && this->layout->processId() == this->pid )
	{
		return this->layout.get();
	}

	return this->refreshMap() ? this->layout.get() : nullptr;
}

std::vector<memmap::Region> MemoryScanner::regions( const memmap::RegionFilter& filter ) const
{
	const MemoryMap* current = this->ensureMap();

	return current == nullptr ? std::vector<memmap::Region>{} : current->select( filter );
}

std::vector<std::uintptr_t> MemoryScanner::scan( const memscan::Pattern& pattern, const memscan::ScanOptions& options ) const
{
	std::vector<std::uintptr_t> found{};

	if ( !pattern.valid() )
	{
		return found;
	}

	const MemoryMap* current = this->ensureMap();

	if ( current == nullptr )
	{
		return found;
	}

	// Normalise scan settings, make sure each chunk is large enough to contain full pattern
	const std::size_t need      = pattern.size();
	const std::size_t alignment = options.alignment == 0 ? 1 : options.alignment;
	const std::size_t chunk     = std::max( options.chunk_bytes, need );

	std::vector<std::uint8_t> buffer{};

	for ( const auto& region : current->regions() )
	{
		// Region too small or not matching filter
		if ( region.size < need || !options.filter.matches( region ) )
		{
			continue;
		}

		std::size_t offset = 0;

		// Read and scan the region in chunks (prevent memory overload)
		while ( offset + need <= region.size )
		{
			const std::size_t want = std::min( chunk, region.size - offset );

			buffer.resize( want );

			const std::size_t transferred = this->readPartial( region.base + offset, buffer.data(), want );

			// Only scan once enough bytes to contain the pattern were loaded
			if ( transferred >= need )
			{
				collectMatches( buffer.data(), transferred, pattern, region.base + offset, alignment, options.max_matches, found );

				if ( options.max_matches != 0 && found.size() >= options.max_matches )
				{
					return found;
				}
			}

			// How far to go before next chunk

			std::size_t advance = 0;

			if ( transferred == 0 )
			{
				// Nothing could be read; skipping
				advance = want;
			}
			else if ( transferred >= need )
			{
				// Keep 1 overhead so patterns crossing chunk boundaries aren't missed
				advance = transferred - ( need - 1 );
			}
			else
			{
				advance = transferred;
			}

			// ALWAYS ADVANCE! in order to prevent infinite memoryread loop (possible system crash!)
			offset += std::max<std::size_t>( advance, 1 );
		}
	}

	return found;
}

std::vector<std::uintptr_t> MemoryScanner::scanBytes( const std::span<const std::uint8_t> data, const memscan::ScanOptions& options ) const
{
	return this->scan( memscan::Pattern::exact( data ), options );
}

std::vector<std::uintptr_t> MemoryScanner::scanString( const std::string_view text, const memscan::ScanOptions& options ) const
{
	return this->scanBytes( std::span{
	                                reinterpret_cast<const std::uint8_t*>( text.data() ),
	                                text.size() },
	                        options );
}

std::vector<std::uintptr_t> MemoryScanner::scanSignature( const std::string_view signature, const memscan::ScanOptions& options ) const
{
	const auto pattern = memscan::Pattern::parse( signature );

	return pattern.has_value() ? this->scan( *pattern, options ) : std::vector<std::uintptr_t>{};
}

std::vector<std::uintptr_t> MemoryScanner::refineBytes( const std::span<const std::uintptr_t> candidates, const std::span<const std::uint8_t> data ) const
{
	std::vector<std::uintptr_t> kept{};

	if ( data.empty() )
	{
		return kept;
	}

	std::vector<std::uint8_t> buffer( data.size() );

	for ( const std::uintptr_t address : candidates )
	{
		if ( !this->read( address, buffer.data(), buffer.size() ) )
		{
			continue;
		}

		if ( std::ranges::equal( buffer, data ) )
		{
			kept.push_back( address );
		}
	}

	return kept;
}

std::vector<memscan::Snapshot> MemoryScanner::snapshot( const memmap::RegionFilter& filter ) const
{
	std::vector<memscan::Snapshot> taken{};

	const MemoryMap* current = this->ensureMap();

	if ( current == nullptr )
	{
		return taken;
	}

	std::vector<std::byte> buffer{};

	for ( const auto& region : current->regions() )
	{
		if ( !filter.matches( region ) )
		{
			continue;
		}

		buffer.resize( region.size );

		if ( this->readPartial( region.base, buffer.data(), region.size ) != region.size )
		{
			continue;
		}

		taken.push_back( memscan::Snapshot{
		        .base       = region.base,
		        .size       = region.size,
		        .protection = region.protection,
		        .path       = region.path,
		        .digest     = Hashing::hashFromData( buffer ) } );
	}

	return taken;
}

std::vector<memscan::Violation> MemoryScanner::verify( const std::span<const memscan::Snapshot> snapshots ) const
{
	std::vector<memscan::Violation> violations{};

	const bool refreshed = this->refreshMap();

	std::vector<std::byte> buffer{};

	for ( const auto& entry : snapshots )
	{
		const memmap::Region* region = refreshed ? this->layout->find( entry.base ) : nullptr;

		if ( region == nullptr || region->base != entry.base )
		{
			violations.push_back( memscan::Violation{
			        .kind   = memscan::ViolationKind::Missing,
			        .base   = entry.base,
			        .size   = entry.size,
			        .path   = entry.path,
			        .detail = "region is no longer mapped at its recorded base" } );

			continue;
		}

		if ( region->size != entry.size )
		{
			violations.push_back( memscan::Violation{
			        .kind   = memscan::ViolationKind::Resized,
			        .base   = entry.base,
			        .size   = region->size,
			        .path   = entry.path,
			        .detail = std::format( "size changed from {} to {}", entry.size, region->size ) } );

			continue;
		}

		if ( region->protection != entry.protection )
		{
			violations.push_back( memscan::Violation{
			        .kind   = memscan::ViolationKind::ProtectionChanged,
			        .base   = entry.base,
			        .size   = entry.size,
			        .path   = entry.path,
			        .detail = std::format( "{} became {}", memmap::toString( entry.protection ), memmap::toString( region->protection ) ) } );
		}

		buffer.resize( entry.size );

		if ( this->readPartial( entry.base, buffer.data(), entry.size ) != entry.size )
		{
			violations.push_back( memscan::Violation{
			        .kind   = memscan::ViolationKind::Unreadable,
			        .base   = entry.base,
			        .size   = entry.size,
			        .path   = entry.path,
			        .detail = "region could no longer be read in full" } );

			continue;
		}

		if ( Hashing::hashFromData( buffer ) != entry.digest )
		{
			violations.push_back( memscan::Violation{
			        .kind   = memscan::ViolationKind::ContentChanged,
			        .base   = entry.base,
			        .size   = entry.size,
			        .path   = entry.path,
			        .detail = "digest no longer matches the recorded snapshot" } );
		}
	}

	return violations;
}
