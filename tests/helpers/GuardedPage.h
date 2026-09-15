//
// Created by mattfor on 8/31/26.
//

#ifndef ABBA_GUARDEDPAGE_H
#define ABBA_GUARDEDPAGE_H

#include <cstdint>
#include <cstddef>

#if defined( _WIN32 )

	#define WIN32_LEAN_AND_MEAN

	#include <windows.h>

#elif defined( __linux__ )

	#include <unistd.h>
	#include <sys/mman.h>

#else
	#error "Unsupported platform"
#endif

// Get system mempage size for alloc granuality
inline std::size_t pageGranularity() noexcept
{
#if defined( __linux__ )
	const long value = sysconf( _SC_PAGESIZE );

	return value > 0 ? static_cast<std::size_t>( value ) : 4096;

#elif defined( _WIN32 )
	SYSTEM_INFO info{};
	GetSystemInfo( &info );

	return static_cast<std::size_t>( info.dwPageSize );
#endif
}

class GuardedPage
{
public:
	GuardedPage()
	    : granularity( pageGranularity() )
	{
		// Allocate three pages
		// [guard page][writeable page][guard page]
		// surrounding guard pages prevent accesses that overflow the usable page from silent successes

#if defined( __linux__ )
		void* raw = mmap( nullptr, this->granularity * 3, PROT_NONE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0 );

		if ( raw == MAP_FAILED )
		{
			return;
		}

		this->block = raw;

		// As written above only middle page is accessible
		auto* middle = static_cast<std::uint8_t*>( raw ) + this->granularity;

		if ( mprotect( middle, this->granularity, PROT_READ | PROT_WRITE ) != 0 )
		{
			munmap( raw, this->granularity * 3 );
			this->block = nullptr;

			return;
		}

		this->page = middle;

#elif defined( _WIN32 )
		// Full 3 page block inaccessible
		void* raw = VirtualAlloc( nullptr, this->granularity * 3, MEM_RESERVE, PAGE_NOACCESS );

		if ( raw == nullptr )
		{
			return;
		}

		this->block = raw;

		auto* middle = static_cast<std::uint8_t*>( raw ) + this->granularity;

		this->page = VirtualAlloc( middle, this->granularity, MEM_COMMIT, PAGE_READWRITE );

		if ( this->page == nullptr )
		{
			VirtualFree( raw, 0, MEM_RELEASE );
			this->block = nullptr;
		}
#endif
	}

	~GuardedPage()
	{
		this->release();
	}

	// Page is used by the OS, abort !!!
	GuardedPage( const GuardedPage& )            = delete;
	GuardedPage& operator=( const GuardedPage& ) = delete;
	GuardedPage( GuardedPage&& )                 = delete;
	GuardedPage& operator=( GuardedPage&& )      = delete;

	[[nodiscard]] bool valid() const noexcept
	{
		return this->page != nullptr;
	}

	[[nodiscard]] std::size_t size() const noexcept
	{
		return this->granularity;
	}

	[[nodiscard]] std::uint8_t* data() const noexcept
	{
		return static_cast<std::uint8_t*>( this->page );
	}

	[[nodiscard]] std::uintptr_t address() const noexcept
	{
		return reinterpret_cast<std::uintptr_t>( this->page );
	}

	[[nodiscard]] bool makeReadOnly() const
	{
		if ( !this->valid() )
		{
			return false;
		}

#if defined( __linux__ )
		return mprotect( this->page, this->granularity, PROT_READ ) == 0;

#elif defined( _WIN32 )
		DWORD previous = 0;

		return VirtualProtect( this->page, this->granularity, PAGE_READONLY, &previous ) != FALSE;
#endif
	}

	void release() noexcept
	{
		if ( this->block == nullptr )
		{
			return;
		}

#if defined( __linux__ )
		munmap( this->block, this->granularity * 3 );

#elif defined( _WIN32 )
		VirtualFree( this->block, 0, MEM_RELEASE );
#endif

		this->block = nullptr;
		this->page  = nullptr;
	}

private:
	void* block{
		nullptr
	};

	void* page{
		nullptr
	};

	std::size_t granularity{};
};

#endif // ABBA_GUARDEDPAGE_H
