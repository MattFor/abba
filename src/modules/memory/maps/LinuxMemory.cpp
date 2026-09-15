//
// Created by mattfor on 8/16/26.
//

#include "modules/memory/maps/LinuxMemory.h"

#if defined( __linux__ )

	#include <string>
	#include <fstream>
	#include <charconv>

	#include "utilities/Logger.h"

namespace
{
[[nodiscard]] std::string_view trimLeft( std::string_view text ) noexcept
{
	while ( !text.empty() && ( text.front() == ' ' || text.front() == '\t' ) )
	{
		text.remove_prefix( 1 );
	}

	return text;
}

[[nodiscard]] std::string_view takeToken( std::string_view& text ) noexcept
{
	text = trimLeft( text );

	const auto space = text.find_first_of( " \t" );

	if ( space == std::string_view::npos )
	{
		const std::string_view token = text;
		text                         = {};

		return token;
	}

	const std::string_view token = text.substr( 0, space );
	text.remove_prefix( space );

	return token;
}

[[nodiscard]] bool parseHex( const std::string_view text, std::uint64_t& value ) noexcept
{
	if ( text.empty() )
	{
		return false;
	}

	const auto* first = text.data();
	const auto* last  = text.data() + text.size();

	const auto [pointer, error] = std::from_chars( first, last, value, 16 );

	return error == std::errc{} && pointer == last;
}

[[nodiscard]] memmap::RegionType classify( const std::string_view path, const bool shared ) noexcept
{
	if ( path.empty() )
	{
		return memmap::RegionType::Private;
	}

	if ( path == "[heap]" )
	{
		return memmap::RegionType::Heap;
	}

	if ( path.starts_with( "[stack" ) )
	{
		return memmap::RegionType::Stack;
	}

	if ( path.starts_with( "[anon" ) )
	{
		return memmap::RegionType::Private;
	}

	if ( path.front() == '[' )
	{
		return memmap::RegionType::Unknown;
	}

	return shared ? memmap::RegionType::Mapped : memmap::RegionType::Image;
}
} // namespace

std::optional<memmap::Region> LinuxMemory::parseLine( const std::string_view line )
{
	std::string_view remainder = line;

	const std::string_view range = takeToken( remainder );

	const auto dash = range.find( '-' );

	if ( dash == std::string_view::npos )
	{
		return std::nullopt;
	}

	std::uint64_t start = 0;
	std::uint64_t stop  = 0;

	if ( !parseHex( range.substr( 0, dash ), start ) || !parseHex( range.substr( dash + 1 ), stop ) || stop < start )
	{
		return std::nullopt;
	}

	const std::string_view permissions = takeToken( remainder );

	if ( permissions.size() < 4 )
	{
		return std::nullopt;
	}

	memmap::Protection protection = memmap::Protection::None;

	if ( permissions[0] == 'r' )
	{
		protection |= memmap::Protection::Read;
	}

	if ( permissions[1] == 'w' )
	{
		protection |= memmap::Protection::Write;
	}

	if ( permissions[2] == 'x' )
	{
		protection |= memmap::Protection::Execute;
	}

	const bool shared = permissions[3] == 's';

	if ( shared )
	{
		protection |= memmap::Protection::Shared;
	}

	std::uint64_t offset = 0;

	if ( !parseHex( takeToken( remainder ), offset ) )
	{
		return std::nullopt;
	}

	static_cast<void>( takeToken( remainder ) );
	static_cast<void>( takeToken( remainder ) );

	const std::string_view path = trimLeft( remainder );

	return memmap::Region{
		.base       = static_cast<std::uintptr_t>( start ),
		.size       = static_cast<std::size_t>( stop - start ),
		.offset     = offset,
		.protection = protection,
		.type       = classify( path, shared ),
		.path       = std::string{
		        path }
	};
}

bool LinuxMemory::collect( const std::uint32_t process_id, std::vector<memmap::Region>& regions ) const
{
	const std::string path = "/proc/" + std::to_string( process_id ) + "/maps";

	std::ifstream stream( path );

	if ( !stream.is_open() )
	{
		p( "[ERROR] MemoryMap could not open {}", path );

		return false;
	}

	std::string line{};

	while ( std::getline( stream, line ) )
	{
		if ( auto region = parseLine( line ); region.has_value() && region->size > 0 )
		{
			regions.push_back( std::move( *region ) );
		}
	}

	return !regions.empty();
}

#endif
