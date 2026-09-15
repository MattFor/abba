//
// Created by mattfor on 8/16/26.
//

#ifndef ABBA_MEMORYMAP_H
#define ABBA_MEMORYMAP_H

#include <memory>
#include <string>
#include <vector>
#include <cstdint>
#include <limits>
#include <optional>
#include <string_view>

namespace memmap
{
enum class Protection : std::uint8_t
{
	None    = 0,
	Read    = 1 << 0,
	Write   = 1 << 1,
	Execute = 1 << 2,
	Shared  = 1 << 3,
	Guard   = 1 << 4
};

[[nodiscard]] constexpr Protection operator|( const Protection left, const Protection right ) noexcept
{
	return static_cast<Protection>( static_cast<std::uint8_t>( left ) | static_cast<std::uint8_t>( right ) );
}

[[nodiscard]] constexpr Protection operator&( const Protection left, const Protection right ) noexcept
{
	return static_cast<Protection>( static_cast<std::uint8_t>( left ) & static_cast<std::uint8_t>( right ) );
}

[[nodiscard]] constexpr Protection operator~( const Protection value ) noexcept
{
	return static_cast<Protection>( static_cast<std::uint8_t>( ~static_cast<std::uint8_t>( value ) ) & 0x1FU );
}

constexpr Protection& operator|=( Protection& left, const Protection right ) noexcept
{
	left = left | right;
	return left;
}

constexpr Protection& operator&=( Protection& left, const Protection right ) noexcept
{
	left = left & right;
	return left;
}

[[nodiscard]] constexpr bool hasAll( const Protection value, const Protection flags ) noexcept
{
	return ( static_cast<std::uint8_t>( value ) & static_cast<std::uint8_t>( flags ) ) == static_cast<std::uint8_t>( flags );
}

[[nodiscard]] constexpr bool hasAny( const Protection value, const Protection flags ) noexcept
{
	return ( static_cast<std::uint8_t>( value ) & static_cast<std::uint8_t>( flags ) ) != 0;
}

[[nodiscard]] std::string toString( Protection protection );

enum class RegionType : std::uint8_t
{
	Unknown = 0,
	Image   = 1,
	Mapped  = 2,
	Private = 3,
	Stack   = 4,
	Heap    = 5
};

[[nodiscard]] std::string_view toString( RegionType type ) noexcept;

struct Region
{
	std::uintptr_t base{};
	std::size_t    size{};
	std::uint64_t  offset{};

	Protection protection{
		Protection::None
	};

	RegionType type{
		RegionType::Unknown
	};

	std::string path{};

	[[nodiscard]] constexpr std::uintptr_t end() const noexcept
	{
		return this->base + this->size;
	}

	[[nodiscard]] constexpr bool contains( const std::uintptr_t address ) const noexcept
	{
		return address >= this->base && address < this->end();
	}

	[[nodiscard]] constexpr bool contains( const std::uintptr_t address, const std::size_t length ) const noexcept
	{
		if ( !this->contains( address ) )
		{
			return false;
		}

		return length <= this->end() - address;
	}

	[[nodiscard]] constexpr bool readable() const noexcept
	{
		return hasAll( this->protection, Protection::Read ) && !hasAny( this->protection, Protection::Guard );
	}

	[[nodiscard]] constexpr bool writable() const noexcept
	{
		return hasAll( this->protection, Protection::Write );
	}

	[[nodiscard]] constexpr bool executable() const noexcept
	{
		return hasAll( this->protection, Protection::Execute );
	}

	[[nodiscard]] constexpr bool anonymous() const noexcept
	{
		return this->path.empty();
	}

	[[nodiscard]] std::string_view name() const noexcept;
};

struct RegionFilter
{
	Protection required{
		Protection::Read
	};

	Protection excluded{
		Protection::Guard
	};

	std::optional<RegionType>  type{};
	std::optional<std::size_t> min_size{};
	std::optional<std::size_t> max_size{};

	std::uintptr_t lowest_address{
		0
	};

	std::uintptr_t highest_address{
		std::numeric_limits<std::uintptr_t>::max()
	};

	std::string name_contains{};

	bool anonymous_only{
		false
	};

	bool named_only{
		false
	};

	[[nodiscard]] bool matches( const Region& region ) const;
};
} // namespace memmap

class MemoryMap
{
  public:
	virtual ~MemoryMap()                         = default;
	MemoryMap( const MemoryMap& )                = delete;
	MemoryMap& operator=( const MemoryMap& )     = delete;
	MemoryMap( MemoryMap&& ) noexcept            = delete;
	MemoryMap& operator=( MemoryMap&& ) noexcept = delete;

	[[nodiscard]] static std::unique_ptr<MemoryMap> create();

	[[nodiscard]] bool refresh( std::uint32_t process_id );
	void               clear() noexcept;

	[[nodiscard]] std::uint32_t processId() const noexcept;
	[[nodiscard]] bool          empty() const noexcept;
	[[nodiscard]] std::size_t   count() const noexcept;
	[[nodiscard]] std::size_t   mappedBytes() const noexcept;

	[[nodiscard]] const std::vector<memmap::Region>& regions() const noexcept;

	[[nodiscard]] const memmap::Region*       find( std::uintptr_t address ) const noexcept;
	[[nodiscard]] const memmap::Region*       image( std::string_view name ) const noexcept;
	[[nodiscard]] std::vector<memmap::Region> select( const memmap::RegionFilter& filter ) const;
	[[nodiscard]] bool                        covers( std::uintptr_t address, std::size_t size, memmap::Protection required ) const noexcept;

  protected:
	MemoryMap() = default;

	[[nodiscard]] virtual bool collect( std::uint32_t process_id, std::vector<memmap::Region>& regions ) const = 0;

  private:
	std::uint32_t pid{};

	std::vector<memmap::Region> mapped{};
};

#endif // ABBA_MEMORYMAP_H
