//
// Created by mattfor on 8/16/26.
//

#include "modules/memory/MemoryScanner.h"

#include <array>
#include <string>
#include <cstdint>
#include <stdexcept>

#if defined(_WIN32)
#include <windows.h>
#elif defined(__linux__)
#include <unistd.h>
#else

#error "Unsupported platform"

#endif


namespace MemoryScannerTests
{
    namespace
    {
        std::uint32_t currentProcessId()
        {
#if defined(_WIN32)
            return static_cast<std::uint32_t>(GetCurrentProcessId());
#elif defined(__linux__)
            return static_cast<std::uint32_t>(getpid());
#endif
        }

        MemoryScanner createAttachedScanner()
        {
            MemoryScanner scanner;

            if (!scanner.attach(currentProcessId()))
            {
                throw std::runtime_error("MemoryScanner::attach() failed for current process");
            }

            return scanner;
        }
    }


    void attach_current_process()
    {
        if (MemoryScanner scanner; !scanner.attach(currentProcessId()))
        {
            throw std::runtime_error("MemoryScanner::attach() failed for current process");
        }
    }


    void attach_current_process_twice()
    {
        MemoryScanner scanner;

        const auto pid = currentProcessId();

        if (!scanner.attach(pid))
        {
            throw std::runtime_error("First attach() failed");
        }

        if (!scanner.attach(pid))
        {
            throw std::runtime_error("Second attach() failed");
        }
    }


    void read_integer()
    {
        const auto scanner = createAttachedScanner();

        constexpr std::uint64_t expected = 0xDEADBEEFCAFEBABEULL;

        const auto address = reinterpret_cast<std::uintptr_t>(&expected);

        std::uint64_t actual{};

        if (!scanner.read(address, &actual, sizeof( actual )))
        {
            throw std::runtime_error("MemoryScanner::read() failed");
        }

        if (actual != expected)
        {
            throw std::runtime_error("MemoryScanner::read() returned incorrect integer");
        }
    }


    void read_single_byte()
    {
        const auto scanner = createAttachedScanner();

        constexpr std::uint8_t expected = 0xAB;

        const auto address = reinterpret_cast<std::uintptr_t>(&expected);

        std::uint8_t actual{};

        if (!scanner.read(address, &actual, sizeof( actual )))
        {
            throw std::runtime_error("MemoryScanner::read() failed for single byte");
        }

        if (actual != expected)
        {
            throw std::runtime_error("MemoryScanner::read() returned incorrect byte");
        }
    }


    void read_buffer()
    {
        const auto scanner = createAttachedScanner();

        constexpr std::array<std::uint8_t, 8> expected{
            0x10,
            0x20,
            0x30,
            0x40,
            0x50,
            0x60,
            0x70,
            0x80
        };

        const auto address = reinterpret_cast<std::uintptr_t>(expected.data());

        std::array<std::uint8_t, expected.size()> actual{};

        if (!scanner.read(address, actual.data(), actual.size()))
        {
            throw std::runtime_error("MemoryScanner::read() failed for buffer");
        }

        if (actual != expected)
        {
            throw std::runtime_error("MemoryScanner::read() returned incorrect buffer");
        }
    }


    void read_string()
    {
        const auto scanner = createAttachedScanner();

        constexpr char expected[] = "ABBA_MEMORY_SCANNER_TEST";

        const auto address = reinterpret_cast<std::uintptr_t>(expected);

        std::array<char, sizeof( expected )> actual{};

        if (!scanner.read(address, actual.data(), actual.size()))
        {
            throw std::runtime_error("MemoryScanner::read() failed for string");
        }

        if (std::string(actual.data()) != expected)
        {
            throw std::runtime_error("MemoryScanner::read() returned incorrect string");
        }
    }


    struct TestData
    {
        std::uint32_t first;
        std::uint64_t second;
        std::uint16_t third;
    };


    void read_struct()
    {
        const auto scanner = createAttachedScanner();

        constexpr TestData expected{
            .first = 0x12345678,
            .second = 0xDEADBEEFCAFEBABEULL,
            .third = 0xBEEF
        };

        const auto address = reinterpret_cast<std::uintptr_t>(&expected);

        TestData actual{};

        if (!scanner.read(address, &actual, sizeof( actual )))
        {
            throw std::runtime_error("MemoryScanner::read() failed for struct");
        }

        if (actual.first != expected.first || actual.second != expected.second || actual.third != expected.third)
        {
            throw std::runtime_error("MemoryScanner::read() returned incorrect struct");
        }
    }


    void read_does_not_overwrite_beyond_size()
    {
        const auto scanner = createAttachedScanner();

        constexpr std::uint32_t expected = 0x12345678;

        const auto address = reinterpret_cast<std::uintptr_t>(&expected);

        constexpr std::uint8_t sentinel = 0xCC;

        std::array<std::uint8_t, 8> buffer{};
        buffer.fill(sentinel);

        if (!scanner.read(address, buffer.data(), sizeof( expected )))
        {
            throw std::runtime_error("MemoryScanner::read() failed");
        }

        const auto* bytes = reinterpret_cast<const std::uint8_t*>(&expected);

        for (std::size_t i = 0; i < sizeof( expected ); ++i)
        {
            if (buffer[i] != bytes[i])
            {
                throw std::runtime_error("MemoryScanner::read() wrote incorrect bytes");
            }
        }

        for (std::size_t i = sizeof( expected ); i < buffer.size(); ++i)
        {
            if (buffer[i] != sentinel)
            {
                throw std::runtime_error("MemoryScanner::read() wrote beyond requested size");
            }
        }
    }


    void reject_invalid_address()
    {
        const auto scanner = createAttachedScanner();

        std::uint64_t value{};

        if (constexpr auto invalidAddress = std::numeric_limits<std::uintptr_t>::max(); scanner.read(invalidAddress, &value, sizeof( value )))
        {
            throw std::runtime_error("MemoryScanner::read() unexpectedly succeeded " "for an invalid address");
        }
    }


    void reject_null_buffer()
    {
        const auto scanner = createAttachedScanner();

        constexpr std::uint64_t value = 123;

        if (const auto address = reinterpret_cast<std::uintptr_t>(&value); scanner.read(address, nullptr, sizeof( value )))
        {
            throw std::runtime_error("MemoryScanner::read() unexpectedly succeeded " "with a null destination buffer");
        }
    }


    void read_zero_bytes()
    {
        const auto scanner = createAttachedScanner();

        constexpr std::uint64_t value = 123;

        if (const auto address = reinterpret_cast<std::uintptr_t>(&value); !scanner.read(address, nullptr, 0))
        {
            throw std::runtime_error("MemoryScanner::read() rejected a zero-byte read");
        }
    }
}
