//
// Created by mattfor on 8/31/26.
//

#include "modules/memory/maps/MemoryMap.h"

#include <string>
#include <cstdint>
#include <stdexcept>

#if defined(_WIN32)
#include "modules/memory/maps/WindowsMemory.h"
#include <windows.h>
#elif defined(__linux__)
#include "modules/memory/maps/LinuxMemory.h"
#include <unistd.h>
#else
#error "Unsupported platform"
#endif

#include "../helpers/GuardedPage.h"

namespace MemoryMapTests
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

        std::unique_ptr<MemoryMap> createRefreshedMap()
        {
            auto map = MemoryMap::create();

            if (!map)
            {
                throw std::runtime_error("MemoryMap::create() returned no provider");
            }

            if (!map->refresh(currentProcessId()))
            {
                throw std::runtime_error("MemoryMap::refresh() failed for current process");
            }

            return map;
        }

        void requirePage(const GuardedPage& page)
        {
            if (!page.valid())
            {
                throw std::runtime_error("Could not reserve a guarded page for the test");
            }
        }
    }

    //NOLINTNEXTLINE
    void refresh_current_process()
    {
        const auto map = createRefreshedMap();

        if (map->empty() || map->count() == 0)
        {
            throw std::runtime_error("MemoryMap::refresh() produced no regions");
        }

        if (map->processId() != currentProcessId())
        {
            throw std::runtime_error("MemoryMap::processId() does not match the refreshed process");
        }

        if (map->mappedBytes() == 0)
        {
            throw std::runtime_error("MemoryMap::mappedBytes() reported nothing mapped");
        }
    }

    //NOLINTNEXTLINE
    void reject_invalid_process()
    {
        const auto map = MemoryMap::create();

        if (!map)
        {
            throw std::runtime_error("MemoryMap::create() returned no provider");
        }

        if (map->refresh(0))
        {
            throw std::runtime_error("MemoryMap::refresh() unexpectedly succeeded for process zero");
        }

        if (!map->empty())
        {
            throw std::runtime_error("MemoryMap retained regions after a failed refresh");
        }
    }

    //NOLINTNEXTLINE
    void regions_are_sorted()
    {
        const auto map = createRefreshedMap();

        const auto& regions = map->regions();

        for (std::size_t index = 1; index < regions.size(); ++index)
        {
            if (regions[index].base < regions[index - 1].base)
            {
                throw std::runtime_error("MemoryMap regions are not ordered by base address");
            }

            if (regions[index].base < regions[index - 1].end())
            {
                throw std::runtime_error("MemoryMap regions overlap");
            }
        }
    }

    //NOLINTNEXTLINE
    void find_known_address()
    {
        const GuardedPage page;
        requirePage(page);

        const auto map = createRefreshedMap();

        const memmap::Region* region = map->find(page.address());

        if (region == nullptr)
        {
            throw std::runtime_error("MemoryMap::find() did not locate the guarded page");
        }

        if (region->base != page.address() || region->size != page.size())
        {
            throw std::runtime_error("MemoryMap::find() returned the wrong region bounds");
        }

        if (!region->readable() || !region->writable() || region->executable())
        {
            throw std::runtime_error("MemoryMap::find() reported the wrong protection");
        }

        if (!region->anonymous() || region->type != memmap::RegionType::Private)
        {
            throw std::runtime_error("MemoryMap::find() reported the wrong region type");
        }
    }

    //NOLINTNEXTLINE
    void reject_unmapped_address()
    {
        const auto map = createRefreshedMap();

        if (map->find(std::numeric_limits<std::uintptr_t>::max()) != nullptr)
        {
            throw std::runtime_error("MemoryMap::find() resolved an unmapped address");
        }

        if (map->find(0) != nullptr)
        {
            throw std::runtime_error("MemoryMap::find() resolved the null address");
        }
    }

    //NOLINTNEXTLINE
    void covers_contiguous_range()
    {
        const GuardedPage page;
        requirePage(page);

        const auto map = createRefreshedMap();

        if (!map->covers(page.address(), page.size(), memmap::Protection::Read | memmap::Protection::Write))
        {
            throw std::runtime_error("MemoryMap::covers() rejected the guarded page");
        }

        if (map->covers(page.address(), page.size() + 1, memmap::Protection::Read))
        {
            throw std::runtime_error("MemoryMap::covers() accepted a range crossing into guarded memory");
        }

        if (map->covers(page.address(), page.size(), memmap::Protection::Execute))
        {
            throw std::runtime_error("MemoryMap::covers() ignored the requested protection");
        }
    }

    //NOLINTNEXTLINE
    void filter_by_protection()
    {
        const auto map = createRefreshedMap();

        const auto executable = map->select(memmap::RegionFilter{
            .required = memmap::Protection::Read | memmap::Protection::Execute
        });

        if (executable.empty())
        {
            throw std::runtime_error("MemoryMap::select() found no executable regions");
        }

        for (const auto& region : executable)
        {
            if (!region.executable() || !region.readable())
            {
                throw std::runtime_error("MemoryMap::select() returned a region that fails the filter");
            }
        }

        if (executable.size() >= map->count())
        {
            throw std::runtime_error("MemoryMap::select() did not narrow the region set");
        }
    }

    //NOLINTNEXTLINE
    void filter_by_address_window()
    {
        const GuardedPage page;
        requirePage(page);

        const auto map = createRefreshedMap();

        const auto window = map->select(memmap::RegionFilter{
            .lowest_address = page.address(),
            .highest_address = page.address()
        });

        if (window.size() != 1)
        {
            throw std::runtime_error("MemoryMap::select() did not isolate the guarded page");
        }

        if (window.front().base != page.address())
        {
            throw std::runtime_error("MemoryMap::select() isolated the wrong region");
        }
    }

    //NOLINTNEXTLINE
    void resolve_own_image()
    {
        const auto map = createRefreshedMap();

        const auto address = reinterpret_cast<std::uintptr_t>(&resolve_own_image);

        const memmap::Region* region = map->find(address);

        if (region == nullptr)
        {
            throw std::runtime_error("MemoryMap::find() did not locate the executing image");
        }

        if (!region->executable() || region->type != memmap::RegionType::Image)
        {
            throw std::runtime_error("MemoryMap classified the executing image incorrectly");
        }

        if (region->path.empty() || region->name().empty())
        {
            throw std::runtime_error("MemoryMap did not record a backing path for the executing image");
        }

        if (map->image(region->name()) == nullptr)
        {
            throw std::runtime_error("MemoryMap::image() could not resolve the executing image by name");
        }

        if (map->image("this-module-does-not-exist") != nullptr)
        {
            throw std::runtime_error("MemoryMap::image() resolved a module that is not mapped");
        }
    }

    //NOLINTNEXTLINE
    void describe_protection()
    {
        if (memmap::toString(memmap::Protection::Read | memmap::Protection::Execute) != "r-xp")
        {
            throw std::runtime_error("memmap::toString() formatted a read-execute mapping incorrectly");
        }

        if (memmap::toString(memmap::Protection::Read | memmap::Protection::Write | memmap::Protection::Shared) != "rw-s")
        {
            throw std::runtime_error("memmap::toString() formatted a shared mapping incorrectly");
        }

        if (memmap::toString(memmap::Protection::None) != "---p")
        {
            throw std::runtime_error("memmap::toString() formatted an inaccessible mapping incorrectly");
        }

        if (memmap::toString(memmap::RegionType::Image) != "image")
        {
            throw std::runtime_error("memmap::toString() named a region type incorrectly");
        }
    }

    //NOLINTNEXTLINE
    void parse_platform_region()
    {
#if defined(__linux__)
        const auto library = LinuxMemory::parseLine("7f8c1c000000-7f8c1c021000 r-xp 00001000 08:02 1234567 /usr/lib/libc.so.6");

        if (!library.has_value())
        {
            throw std::runtime_error("LinuxMemory::parseLine() rejected a valid mapping");
        }

        if (library->base != 0x7f8c1c000000ULL || library->size != 0x21000 || library->offset != 0x1000)
        {
            throw std::runtime_error("LinuxMemory::parseLine() decoded the wrong bounds");
        }

        if (!library->readable() || !library->executable() || library->writable())
        {
            throw std::runtime_error("LinuxMemory::parseLine() decoded the wrong protection");
        }

        if (library->type != memmap::RegionType::Image || library->path != "/usr/lib/libc.so.6" || library->name() != "libc.so.6")
        {
            throw std::runtime_error("LinuxMemory::parseLine() decoded the wrong backing file");
        }

        const auto heap = LinuxMemory::parseLine("55d4e0000000-55d4e0021000 rw-p 00000000 00:00 0 [heap]");

        if (!heap.has_value() || heap->type != memmap::RegionType::Heap)
        {
            throw std::runtime_error("LinuxMemory::parseLine() did not recognise the heap");
        }

        const auto anonymous = LinuxMemory::parseLine("55d4e0021000-55d4e0022000 rw-p 00000000 00:00 0");

        if (!anonymous.has_value() || anonymous->type != memmap::RegionType::Private || !anonymous->anonymous())
        {
            throw std::runtime_error("LinuxMemory::parseLine() did not recognise an anonymous mapping");
        }

        const auto shared = LinuxMemory::parseLine("7f8c1c100000-7f8c1c101000 rw-s 00000000 00:05 42 /dev/shm/example");

        if (!shared.has_value() || shared->type != memmap::RegionType::Mapped || !memmap::hasAll(shared->protection, memmap::Protection::Shared))
        {
            throw std::runtime_error("LinuxMemory::parseLine() did not recognise a shared mapping");
        }

        if (LinuxMemory::parseLine("not a mapping").has_value())
        {
            throw std::runtime_error("LinuxMemory::parseLine() accepted malformed input");
        }

#elif defined(_WIN32)
        if (WindowsMemory::translateProtection(PAGE_EXECUTE_READ) != ( memmap::Protection::Read | memmap::Protection::Execute ))
        {
            throw std::runtime_error("WindowsMemory::translateProtection() decoded PAGE_EXECUTE_READ incorrectly");
        }

        if (WindowsMemory::translateProtection(PAGE_READWRITE) != ( memmap::Protection::Read | memmap::Protection::Write ))
        {
            throw std::runtime_error("WindowsMemory::translateProtection() decoded PAGE_READWRITE incorrectly");
        }

        if (!memmap::hasAll(WindowsMemory::translateProtection(PAGE_READONLY | PAGE_GUARD), memmap::Protection::Guard))
        {
            throw std::runtime_error("WindowsMemory::translateProtection() lost the guard flag");
        }

        if (WindowsMemory::translateProtection(PAGE_NOACCESS) != memmap::Protection::None)
        {
            throw std::runtime_error("WindowsMemory::translateProtection() decoded PAGE_NOACCESS incorrectly");
        }

        if (WindowsMemory::translateType(MEM_IMAGE) != memmap::RegionType::Image || WindowsMemory::translateType(MEM_PRIVATE) != memmap::RegionType::Private)
        {
            throw std::runtime_error("WindowsMemory::translateType() decoded a region type incorrectly");
        }
#endif
    }
}
