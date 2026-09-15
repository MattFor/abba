//
// Created by mattfor on 8/16/26.
//

#ifndef ABBA_MEMORYSCANNER_H
#define ABBA_MEMORYSCANNER_H

#include <span>
#include <memory>
#include <string>
#include <vector>
#include <cstring>
#include <cstdint>
#include <optional>
#include <type_traits>
#include <string_view>

#include "maps/MemoryMap.h"

namespace memscan
{
struct Pattern
{
	std::vector<std::uint8_t> bytes{};
	std::vector<std::uint8_t> mask{};

	[[nodiscard]] static std::optional<Pattern> parse( std::string_view signature );
	[[nodiscard]] static Pattern                exact( std::span<const std::uint8_t> data );

	[[nodiscard]] std::size_t size() const noexcept
	{
		return this->bytes.size();
	}

	[[nodiscard]] bool empty() const noexcept
	{
		return this->bytes.empty();
	}

	[[nodiscard]] bool valid() const noexcept;
	[[nodiscard]] bool matches( const std::uint8_t* data ) const noexcept;
};

struct ScanOptions
{
	memmap::RegionFilter filter{};

	std::size_t chunk_bytes{
		1U << 20U
	};

	std::size_t alignment{
		1
	};

	std::size_t max_matches{
		0
	};
};

struct Snapshot
{
	std::uintptr_t base{};
	std::size_t    size{};

	memmap::Protection protection{
		memmap::Protection::None
	};

	std::string path{};
	std::string digest{};
};

enum class ViolationKind : std::uint8_t
{
	Missing           = 0,
	Resized           = 1,
	ProtectionChanged = 2,
	ContentChanged    = 3,
	Unreadable        = 4
};

[[nodiscard]] std::string_view toString( ViolationKind kind ) noexcept;

struct Violation
{
	ViolationKind kind{};

	std::uintptr_t base{};
	std::size_t    size{};

	std::string path{};
	std::string detail{};
};
} // namespace memscan

class MemoryScanner
{
  public:
	MemoryScanner();
	~MemoryScanner();

	MemoryScanner( const MemoryScanner& )            = delete;
	MemoryScanner& operator=( const MemoryScanner& ) = delete;

	MemoryScanner( MemoryScanner&& other ) noexcept;
	MemoryScanner& operator=( MemoryScanner&& other ) noexcept;

	[[nodiscard]] bool attach( std::uint32_t process_id );
	void               detach() noexcept;

	[[nodiscard]] bool          isAttached() const noexcept;
	[[nodiscard]] bool          isWritable() const noexcept;
	[[nodiscard]] bool          isRunning() const noexcept;
	[[nodiscard]] std::uint32_t processId() const noexcept;

	[[nodiscard]] bool        read( std::uintptr_t address, void* buffer, std::size_t size ) const;
	[[nodiscard]] std::size_t readPartial( std::uintptr_t address, void* buffer, std::size_t size ) const;
	[[nodiscard]] bool        write( std::uintptr_t address, const void* buffer, std::size_t size ) const;

	template <typename T>
	[[nodiscard]] std::optional<T> read( const std::uintptr_t address ) const
	{
		// BTW this just means the type can be copied byte for byte without issues :)
		static_assert( std::is_trivially_copyable_v<T>, "MemoryScanner::read requires a trivially copyable type" );

		T value{};

		if ( !this->read( address, &value, sizeof( T ) ) )
		{
			return std::nullopt;
		}

		return value;
	}

	template <typename T>
	[[nodiscard]] bool write( const std::uintptr_t address, const T& value ) const
	{
		static_assert( std::is_trivially_copyable_v<T>, "MemoryScanner::write requires a trivially copyable type" );

		return this->write( address, &value, sizeof( T ) );
	}

	[[nodiscard]] std::optional<std::vector<std::uint8_t>> readBytes( std::uintptr_t address, std::size_t size ) const;
	[[nodiscard]] std::optional<std::string>               readString( std::uintptr_t address, std::size_t max_length ) const;

	[[nodiscard]] bool             refreshMap() const;
	[[nodiscard]] const MemoryMap* map() const noexcept;

	[[nodiscard]] std::vector<memmap::Region> regions( const memmap::RegionFilter& filter = {} ) const;

	[[nodiscard]] std::vector<std::uintptr_t> scan( const memscan::Pattern& pattern, const memscan::ScanOptions& options = {} ) const;
	[[nodiscard]] std::vector<std::uintptr_t> scanBytes( std::span<const std::uint8_t> data, const memscan::ScanOptions& options = {} ) const;
	[[nodiscard]] std::vector<std::uintptr_t> scanString( std::string_view text, const memscan::ScanOptions& options = {} ) const;
	[[nodiscard]] std::vector<std::uintptr_t> scanSignature( std::string_view signature, const memscan::ScanOptions& options = {} ) const;

	template <typename T>
	[[nodiscard]] std::vector<std::uintptr_t> scanValue( const T& value, const memscan::ScanOptions& options = {} ) const
	{
		static_assert( std::is_trivially_copyable_v<T>, "MemoryScanner::scanValue requires a trivially copyable type" );

		std::vector<std::uint8_t> raw( sizeof( T ) );
		std::memcpy( raw.data(), &value, sizeof( T ) );

		return this->scanBytes( raw, options );
	}

	[[nodiscard]] std::vector<std::uintptr_t> refineBytes( std::span<const std::uintptr_t> candidates, std::span<const std::uint8_t> data ) const;

	template <typename T>
	[[nodiscard]] std::vector<std::uintptr_t> refine( const std::span<const std::uintptr_t> candidates, const T& value ) const
	{
		static_assert( std::is_trivially_copyable_v<T>, "MemoryScanner::refine requires a trivially copyable type" );

		std::vector<std::uint8_t> raw( sizeof( T ) );
		std::memcpy( raw.data(), &value, sizeof( T ) );

		return this->refineBytes( candidates, raw );
	}

	[[nodiscard]] std::vector<memscan::Snapshot>  snapshot( const memmap::RegionFilter& filter = {} ) const;
	[[nodiscard]] std::vector<memscan::Violation> verify( std::span<const memscan::Snapshot> snapshots ) const;

  private:
	[[nodiscard]] const MemoryMap* ensureMap() const;

	std::uint32_t pid{};

	void* handle{
		nullptr
	};

	bool writable{
		false
	};

	mutable std::unique_ptr<MemoryMap> layout{};
};

#endif // ABBA_MEMORYSCANNER_H
