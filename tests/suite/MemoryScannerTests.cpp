//
// Created by mattfor on 8/16/26.
//

#include "modules/memory/MemoryScanner.h"

#include <array>
#include <string>
#include <limits>
#include <cstdint>
#include <numeric>
#include <algorithm>
#include <stdexcept>

#if defined(_WIN32)
#include <windows.h>
#elif defined(__linux__)
#include <unistd.h>
#else

#error "Unsupported platform"

#endif

#include "../helpers/GuardedPage.h"


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

    //NOLINTNEXTLINE
    void attach_current_process()
    {
        if (MemoryScanner scanner; !scanner.attach(currentProcessId()))
        {
            throw std::runtime_error("MemoryScanner::attach() failed for current process");
        }
    }

    //NOLINTNEXTLINE
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

    //NOLINTNEXTLINE
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

    //NOLINTNEXTLINE
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

    //NOLINTNEXTLINE
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

    //NOLINTNEXTLINE
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


    namespace
    {
        struct TestData
        {
            std::uint32_t first;
            std::uint64_t second;
            std::uint16_t third;
        };
    }

    //NOLINTNEXTLINE
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

    //NOLINTNEXTLINE
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

    //NOLINTNEXTLINE
    void reject_invalid_address()
    {
        const auto scanner = createAttachedScanner();

        std::uint64_t value{};

        if (constexpr auto invalidAddress = std::numeric_limits<std::uintptr_t>::max(); scanner.read(invalidAddress, &value, sizeof( value )))
        {
            throw std::runtime_error("MemoryScanner::read() unexpectedly succeeded " "for an invalid address");
        }
    }

    //NOLINTNEXTLINE
    void reject_null_buffer()
    {
        const auto scanner = createAttachedScanner();

        constexpr std::uint64_t value = 123;

        if (const auto address = reinterpret_cast<std::uintptr_t>(&value); scanner.read(address, nullptr, sizeof( value )))
        {
            throw std::runtime_error("MemoryScanner::read() unexpectedly succeeded " "with a null destination buffer");
        }
    }

    //NOLINTNEXTLINE
    void read_zero_bytes()
    {
        const auto scanner = createAttachedScanner();

        constexpr std::uint64_t value = 123;

        if (const auto address = reinterpret_cast<std::uintptr_t>(&value); !scanner.read(address, nullptr, 0))
        {
            throw std::runtime_error("MemoryScanner::read() rejected a zero-byte read");
        }
    }

    namespace
    {
        constexpr std::array<std::uint8_t, 16> kMarker{
            0xAB,
            0xBA,
            0x13,
            0x37,
            0xDE,
            0xAD,
            0xC0,
            0xDE,
            0xFE,
            0xED,
            0xFA,
            0xCE,
            0x0B,
            0xAD,
            0xF0,
            0x0D
        };

        void requirePage(const GuardedPage& page)
        {
            if (!page.valid())
            {
                throw std::runtime_error("Could not reserve a guarded page for the test");
            }
        }

        memscan::ScanOptions optionsFor(const GuardedPage& page)
        {
            return memscan::ScanOptions{
                .filter = memmap::RegionFilter{
                    .lowest_address = page.address(),
                    .highest_address = page.address()
                }
            };
        }

        void placeMarker(const GuardedPage& page, const std::size_t offset)
        {
            std::ranges::copy(kMarker, page.data() + offset);
        }
    }

    //NOLINTNEXTLINE
    void detach_releases_process()
    {
        MemoryScanner scanner;

        if (scanner.isAttached() || scanner.processId() != 0)
        {
            throw std::runtime_error("A fresh MemoryScanner reported itself as attached");
        }

        if (!scanner.attach(currentProcessId()))
        {
            throw std::runtime_error("MemoryScanner::attach() failed for current process");
        }

        if (!scanner.isAttached() || scanner.processId() != currentProcessId())
        {
            throw std::runtime_error("MemoryScanner did not record the attached process");
        }

        scanner.detach();

        if (scanner.isAttached() || scanner.processId() != 0 || scanner.isRunning())
        {
            throw std::runtime_error("MemoryScanner remained attached after detach()");
        }

        std::uint64_t value{};

        if (scanner.read(reinterpret_cast<std::uintptr_t>(&value), &value, sizeof( value )))
        {
            throw std::runtime_error("MemoryScanner::read() succeeded while detached");
        }
    }

    //NOLINTNEXTLINE
    void reject_invalid_process()
    {
        MemoryScanner scanner;

        if (scanner.attach(0))
        {
            throw std::runtime_error("MemoryScanner::attach() accepted process zero");
        }

        if (scanner.attach(0x7FFFFFFFU))
        {
            throw std::runtime_error("MemoryScanner::attach() accepted a process that does not exist");
        }

        if (scanner.isAttached())
        {
            throw std::runtime_error("MemoryScanner reported an attachment after a failed attach()");
        }
    }

    //NOLINTNEXTLINE
    void reports_running_process()
    {
        const auto scanner = createAttachedScanner();

        if (!scanner.isRunning())
        {
            throw std::runtime_error("MemoryScanner::isRunning() reported the current process as gone");
        }

        if (!scanner.isWritable())
        {
            throw std::runtime_error("MemoryScanner::isWritable() denied write access to the current process");
        }
    }

    //NOLINTNEXTLINE
    void read_typed_value()
    {
        const auto scanner = createAttachedScanner();

        constexpr std::uint64_t expected = 0xDEADBEEFCAFEBABEULL;

        const auto value = scanner.read<std::uint64_t>(reinterpret_cast<std::uintptr_t>(&expected));

        if (!value.has_value() || *value != expected)
        {
            throw std::runtime_error("MemoryScanner::read<T>() returned an incorrect value");
        }

        if (scanner.read<std::uint64_t>(std::numeric_limits<std::uintptr_t>::max()).has_value())
        {
            throw std::runtime_error("MemoryScanner::read<T>() succeeded for an invalid address");
        }
    }

    //NOLINTNEXTLINE
    void read_byte_span()
    {
        const auto scanner = createAttachedScanner();

        const auto bytes = scanner.readBytes(reinterpret_cast<std::uintptr_t>(kMarker.data()), kMarker.size());

        if (!bytes.has_value() || !std::ranges::equal(*bytes, kMarker))
        {
            throw std::runtime_error("MemoryScanner::readBytes() returned incorrect data");
        }

        if (scanner.readBytes(std::numeric_limits<std::uintptr_t>::max(), kMarker.size()).has_value())
        {
            throw std::runtime_error("MemoryScanner::readBytes() succeeded for an invalid address");
        }
    }

    //NOLINTNEXTLINE
    void read_null_terminated_string()
    {
        const GuardedPage page;
        requirePage(page);

        constexpr std::string_view expected = "abba-memory-scanner";

        std::ranges::copy(expected, reinterpret_cast<char*>(page.data()));
        page.data()[expected.size()] = 0;

        const auto scanner = createAttachedScanner();

        const auto text = scanner.readString(page.address(), 64);

        if (!text.has_value() || *text != expected)
        {
            throw std::runtime_error("MemoryScanner::readString() returned incorrect text");
        }

        if (scanner.readString(std::numeric_limits<std::uintptr_t>::max(), 64).has_value())
        {
            throw std::runtime_error("MemoryScanner::readString() succeeded for an invalid address");
        }
    }

    //NOLINTNEXTLINE
    void write_typed_value()
    {
        const GuardedPage page;
        requirePage(page);

        const auto scanner = createAttachedScanner();

        constexpr std::uint32_t expected = 0xA5A5A5A5U;

        if (!scanner.write<std::uint32_t>(page.address(), expected))
        {
            throw std::runtime_error("MemoryScanner::write<T>() failed");
        }

        std::uint32_t observed{};
        std::memcpy(&observed, page.data(), sizeof( observed ));

        if (observed != expected)
        {
            throw std::runtime_error("MemoryScanner::write<T>() did not store the value");
        }

        if (scanner.write<std::uint32_t>(std::numeric_limits<std::uintptr_t>::max(), expected))
        {
            throw std::runtime_error("MemoryScanner::write<T>() succeeded for an invalid address");
        }
    }

    //NOLINTNEXTLINE
    void pattern_parses_wildcards()
    {
        const auto pattern = memscan::Pattern::parse("48 8B ?? ?? 89 C3");

        if (!pattern.has_value() || pattern->size() != 6 || !pattern->valid())
        {
            throw std::runtime_error("Pattern::parse() rejected a valid signature");
        }

        if (pattern->mask[0] != 0xFF || pattern->mask[2] != 0x00 || pattern->mask[3] != 0x00 || pattern->mask[5] != 0xFF)
        {
            throw std::runtime_error("Pattern::parse() built an incorrect mask");
        }

        if (pattern->bytes[0] != 0x48 || pattern->bytes[1] != 0x8B || pattern->bytes[4] != 0x89)
        {
            throw std::runtime_error("Pattern::parse() decoded incorrect bytes");
        }

        constexpr std::array<std::uint8_t, 6> subject{
            0x48,
            0x8B,
            0x00,
            0xFF,
            0x89,
            0xC3
        };

        if (!pattern->matches(subject.data()))
        {
            throw std::runtime_error("Pattern::matches() rejected a subject the signature covers");
        }
    }

    //NOLINTNEXTLINE
    void pattern_matches_nibble_wildcard()
    {
        const auto pattern = memscan::Pattern::parse("4? ?8");

        if (!pattern.has_value() || pattern->mask[0] != 0xF0 || pattern->mask[1] != 0x0F)
        {
            throw std::runtime_error("Pattern::parse() did not honour nibble wildcards");
        }

        constexpr std::array<std::uint8_t, 2> matching{
            0x4C,
            0xB8
        };

        constexpr std::array<std::uint8_t, 2> differing{
            0x5C,
            0xB8
        };

        if (!pattern->matches(matching.data()) || pattern->matches(differing.data()))
        {
            throw std::runtime_error("Pattern::matches() mishandled a nibble wildcard");
        }
    }

    //NOLINTNEXTLINE
    void pattern_rejects_malformed_signature()
    {
        if (memscan::Pattern::parse("").has_value())
        {
            throw std::runtime_error("Pattern::parse() accepted an empty signature");
        }

        if (memscan::Pattern::parse("?? ??").has_value())
        {
            throw std::runtime_error("Pattern::parse() accepted a signature made only of wildcards");
        }

        if (memscan::Pattern::parse("48 ZZ").has_value())
        {
            throw std::runtime_error("Pattern::parse() accepted a non-hexadecimal token");
        }

        if (memscan::Pattern::parse("48 8B4C").has_value())
        {
            throw std::runtime_error("Pattern::parse() accepted an oversized token");
        }
    }

    //NOLINTNEXTLINE
    void scan_finds_marker()
    {
        const GuardedPage page;
        requirePage(page);

        placeMarker(page, 64);

        const auto scanner = createAttachedScanner();

        const auto found = scanner.scanBytes(kMarker, optionsFor(page));

        if (found.size() != 1 || found.front() != page.address() + 64)
        {
            throw std::runtime_error("MemoryScanner::scanBytes() did not locate the marker exactly once");
        }

        const auto absent = scanner.scanString("this-sequence-is-not-present-in-the-page", optionsFor(page));

        if (!absent.empty())
        {
            throw std::runtime_error("MemoryScanner::scanString() reported a match that does not exist");
        }
    }

    //NOLINTNEXTLINE
    void scan_signature_with_wildcards()
    {
        const GuardedPage page;
        requirePage(page);

        placeMarker(page, 128);

        const auto scanner = createAttachedScanner();

        const auto found = scanner.scanSignature("AB BA ?? ?? DE AD C0 DE", optionsFor(page));

        if (found.size() != 1 || found.front() != page.address() + 128)
        {
            throw std::runtime_error("MemoryScanner::scanSignature() did not locate the marker");
        }

        if (!scanner.scanSignature("not a signature", optionsFor(page)).empty())
        {
            throw std::runtime_error("MemoryScanner::scanSignature() scanned with a malformed signature");
        }
    }

    //NOLINTNEXTLINE
    void scan_respects_alignment()
    {
        const GuardedPage page;
        requirePage(page);

        placeMarker(page, 65);

        const auto scanner = createAttachedScanner();

        auto options = optionsFor(page);

        options.alignment = 1;

        if (scanner.scan(memscan::Pattern::exact(kMarker), options).size() != 1)
        {
            throw std::runtime_error("MemoryScanner::scan() missed an unaligned marker");
        }

        options.alignment = 2;

        if (!scanner.scan(memscan::Pattern::exact(kMarker), options).empty())
        {
            throw std::runtime_error("MemoryScanner::scan() ignored the requested alignment");
        }
    }

    //NOLINTNEXTLINE
    void scan_respects_match_limit()
    {
        const GuardedPage page;
        requirePage(page);

        placeMarker(page, 64);
        placeMarker(page, 512);

        const auto scanner = createAttachedScanner();

        auto options = optionsFor(page);

        if (scanner.scanBytes(kMarker, options).size() != 2)
        {
            throw std::runtime_error("MemoryScanner::scanBytes() did not find both markers");
        }

        options.max_matches = 1;

        if (scanner.scanBytes(kMarker, options).size() != 1)
        {
            throw std::runtime_error("MemoryScanner::scanBytes() ignored the match limit");
        }
    }

    //NOLINTNEXTLINE
    void scan_finds_typed_value()
    {
        const GuardedPage page;
        requirePage(page);

        constexpr std::uint32_t needle = 0x0BADF00DU;

        std::memcpy(page.data() + 256, &needle, sizeof( needle ));

        const auto scanner = createAttachedScanner();

        const auto found = scanner.scanValue(needle, optionsFor(page));

        if (found.size() != 1 || found.front() != page.address() + 256)
        {
            throw std::runtime_error("MemoryScanner::scanValue() did not locate the value");
        }
    }

    //NOLINTNEXTLINE
    void refine_narrows_candidates()
    {
        const GuardedPage page;
        requirePage(page);

        constexpr std::uint32_t kept    = 0x11223344U;
        constexpr std::uint32_t dropped = 0x55667788U;

        std::memcpy(page.data() + 32, &kept, sizeof( kept ));
        std::memcpy(page.data() + 64, &dropped, sizeof( dropped ));

        const auto scanner = createAttachedScanner();

        const std::array<std::uintptr_t, 3> candidates{
            page.address() + 32,
            page.address() + 64,
            std::numeric_limits<std::uintptr_t>::max()
        };

        const auto survivors = scanner.refine(candidates, kept);

        if (survivors.size() != 1 || survivors.front() != page.address() + 32)
        {
            throw std::runtime_error("MemoryScanner::refine() kept the wrong candidates");
        }
    }

    //NOLINTNEXTLINE
    void map_isolates_region()
    {
        const GuardedPage page;
        requirePage(page);

        const auto scanner = createAttachedScanner();

        const auto regions = scanner.regions(memmap::RegionFilter{
            .lowest_address = page.address(),
            .highest_address = page.address()
        });

        if (regions.size() != 1 || regions.front().base != page.address() || regions.front().size != page.size())
        {
            throw std::runtime_error("MemoryScanner::regions() did not isolate the guarded page");
        }

        if (scanner.map() == nullptr || scanner.map()->processId() != currentProcessId())
        {
            throw std::runtime_error("MemoryScanner::map() did not expose the refreshed map");
        }
    }

    //NOLINTNEXTLINE
    void snapshot_accepts_untouched_region()
    {
        const GuardedPage page;
        requirePage(page);

        placeMarker(page, 0);

        const auto scanner = createAttachedScanner();

        const auto snapshots = scanner.snapshot(optionsFor(page).filter);

        if (snapshots.size() != 1 || snapshots.front().base != page.address() || snapshots.front().digest.empty())
        {
            throw std::runtime_error("MemoryScanner::snapshot() did not record the guarded page");
        }

        if (!scanner.verify(snapshots).empty())
        {
            throw std::runtime_error("MemoryScanner::verify() reported a violation for untouched memory");
        }
    }

    //NOLINTNEXTLINE
    void snapshot_detects_content_change()
    {
        const GuardedPage page;
        requirePage(page);

        const auto scanner = createAttachedScanner();

        const auto snapshots = scanner.snapshot(optionsFor(page).filter);

        if (snapshots.size() != 1)
        {
            throw std::runtime_error("MemoryScanner::snapshot() did not record the guarded page");
        }

        page.data()[7] = 0x42;

        const auto violations = scanner.verify(snapshots);

        if (violations.size() != 1 || violations.front().kind != memscan::ViolationKind::ContentChanged)
        {
            throw std::runtime_error("MemoryScanner::verify() did not report the tampered content");
        }

        if (violations.front().base != page.address() || violations.front().detail.empty())
        {
            throw std::runtime_error("MemoryScanner::verify() reported the violation without usable details");
        }
    }

    //NOLINTNEXTLINE
    void snapshot_detects_protection_change()
    {
        const GuardedPage page;
        requirePage(page);

        const auto scanner = createAttachedScanner();

        const auto snapshots = scanner.snapshot(optionsFor(page).filter);

        if (snapshots.size() != 1)
        {
            throw std::runtime_error("MemoryScanner::snapshot() did not record the guarded page");
        }

        if (!page.makeReadOnly())
        {
            throw std::runtime_error("Could not change the protection of the guarded page");
        }

        const auto violations = scanner.verify(snapshots);

        if (violations.size() != 1 || violations.front().kind != memscan::ViolationKind::ProtectionChanged)
        {
            throw std::runtime_error("MemoryScanner::verify() did not report the protection change");
        }
    }

    //NOLINTNEXTLINE
    void snapshot_detects_unmapped_region()
    {
        GuardedPage page;
        requirePage(page);

        const auto scanner = createAttachedScanner();

        const auto snapshots = scanner.snapshot(optionsFor(page).filter);

        if (snapshots.size() != 1)
        {
            throw std::runtime_error("MemoryScanner::snapshot() did not record the guarded page");
        }

        page.release();

        const auto violations = scanner.verify(snapshots);

        if (violations.size() != 1 || violations.front().kind != memscan::ViolationKind::Missing)
        {
            throw std::runtime_error("MemoryScanner::verify() did not report the unmapped region");
        }

        if (memscan::toString(violations.front().kind) != "missing")
        {
            throw std::runtime_error("memscan::toString() named the violation incorrectly");
        }
    }
}
