//
// Created by mattfor on 8/16/26.
//

#include <cctype>
#include <algorithm>

#include "modules/memory/maps/MemoryMap.h"

#if defined( _WIN32 )
	#include "modules/memory/maps/WindowsMemory.h"
#elif defined( __linux__ )
	#include "modules/memory/maps/LinuxMemory.h"
#else
	#error "Unsupported platform"
#endif

namespace
{
[[nodiscard]] char lowerAscii( const char character ) noexcept
{
	return static_cast<char>( std::tolower( static_cast<unsigned char>( character ) ) );
}

[[nodiscard]] bool containsInsensitive( const std::string_view haystack, const std::string_view needle ) noexcept
{
	if ( needle.empty() )
	{
		return true;
	}

	if ( needle.size() > haystack.size() )
	{
		return false;
	}

	// :mouth_with_tongue_out:
	const auto found = std::ranges::search( haystack, needle, []( const char left, const char right )
	                                        { return lowerAscii( left ) == lowerAscii( right ); } );

	return !found.empty();
}
} // namespace

namespace memmap
{
std::string toString( const Protection protection )
{
	std::string text{};
	text.reserve( 5 );

	text.push_back( hasAll( protection, Protection::Read ) ? 'r' : '-' );
	text.push_back( hasAll( protection, Protection::Write ) ? 'w' : '-' );
	text.push_back( hasAll( protection, Protection::Execute ) ? 'x' : '-' );
	text.push_back( hasAll( protection, Protection::Shared ) ? 's' : 'p' );

	if ( hasAll( protection, Protection::Guard ) )
	{
		text.push_back( 'g' );
	}

	return text;
}

std::string_view toString( const RegionType type ) noexcept
{
	switch ( type )
	{
		case RegionType::Image:
			return "image";

		case RegionType::Mapped:
			return "mapped";

		case RegionType::Private:
			return "private";

		case RegionType::Stack:
			return "stack";

		case RegionType::Heap:
			return "heap";

		case RegionType::Unknown:
		default:
			return "unknown";
	}
}

std::string_view Region::name() const noexcept
{
	const std::string_view full{
		this->path
	};

	if ( const auto separator = full.find_last_of( "/\\" ); separator != std::string_view::npos )
	{
		return full.substr( separator + 1 );
	}

	return full;
}

bool RegionFilter::matches( const Region& region ) const
{
	if ( region.size == 0 )
	{
		return false;
	}

	if ( !hasAll( region.protection, this->required ) )
	{
		return false;
	}

	if ( hasAny( region.protection, this->excluded ) )
	{
		return false;
	}

	if ( this->type.has_value() && *this->type != region.type )
	{
		return false;
	}

	if ( this->min_size.has_value() && region.size < *this->min_size )
	{
		return false;
	}

	if ( this->max_size.has_value() && region.size > *this->max_size )
	{
		return false;
	}

	if ( region.end() <= this->lowest_address || region.base > this->highest_address )
	{
		return false;
	}

	if ( this->anonymous_only && !region.anonymous() )
	{
		return false;
	}

	if ( this->named_only && region.anonymous() )
	{
		return false;
	}

	return containsInsensitive( region.path, this->name_contains );
}
} // namespace memmap

std::unique_ptr<MemoryMap> MemoryMap::create()
{
#if defined( _WIN32 )
	return std::make_unique<WindowsMemory>();
#elif defined( __linux__ )
	return std::make_unique<LinuxMemory>();
#else
	return nullptr;
#endif
}

bool MemoryMap::refresh( const std::uint32_t process_id )
{
	std::vector<memmap::Region> collected{};

	if ( !this->collect( process_id, collected ) )
	{
		return false;
	}

	std::ranges::sort( collected, {}, &memmap::Region::base );

	this->pid    = process_id;
	this->mapped = std::move( collected );

	return true;
}

void MemoryMap::clear() noexcept
{
	this->pid = 0;
	this->mapped.clear();
}

std::uint32_t MemoryMap::processId() const noexcept
{
	return this->pid;
}

bool MemoryMap::empty() const noexcept
{
	return this->mapped.empty();
}

std::size_t MemoryMap::count() const noexcept
{
	return this->mapped.size();
}

std::size_t MemoryMap::mappedBytes() const noexcept
{
	std::size_t total = 0;

	for ( const auto& region : this->mapped )
	{
		total += region.size;
	}

	return total;
}

const std::vector<memmap::Region>& MemoryMap::regions() const noexcept
{
	return this->mapped;
}

const memmap::Region* MemoryMap::find( const std::uintptr_t address ) const noexcept
{
	const auto after = std::ranges::upper_bound( this->mapped, address, {}, &memmap::Region::base );

	if ( after == this->mapped.begin() )
	{
		return nullptr;
	}

	const auto candidate = std::prev( after );

	return candidate->contains( address ) ? &*candidate : nullptr;
}

const memmap::Region* MemoryMap::image( const std::string_view name ) const noexcept
{
	for ( const auto& region : this->mapped )
	{
		if ( region.type == memmap::RegionType::Image && containsInsensitive( region.name(), name ) )
		{
			return &region;
		}
	}

	return nullptr;
}

std::vector<memmap::Region> MemoryMap::select( const memmap::RegionFilter& filter ) const
{
	std::vector<memmap::Region> selected{};

	for ( const auto& region : this->mapped )
	{
		if ( filter.matches( region ) )
		{
			selected.push_back( region );
		}
	}

	return selected;
}

bool MemoryMap::covers( const std::uintptr_t address, const std::size_t size, const memmap::Protection required ) const noexcept
{
	// For a 0 region just see if region is mapped at all
	if ( size == 0 )
	{
		return this->find( address ) != nullptr;
	}

	std::uintptr_t cursor    = address;
	std::size_t    remaining = size;

	// Go through all requested regions
	while ( remaining > 0 )
	{
		const memmap::Region* region = this->find( cursor );

		// Region must be mapped and contain proper protection flags
		if ( region == nullptr || !memmap::hasAll( region->protection, required ) )
		{
			return false;
		}

		const std::size_t available = region->end() - cursor;

		// Region covers rest of requested range
		if ( available >= remaining )
		{
			return true;
		}

		// Move to next region and check remained of the rnage
		cursor += available;
		remaining -= available;
	}

	return true;
}
