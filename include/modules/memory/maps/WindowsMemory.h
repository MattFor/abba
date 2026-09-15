//
// Created by mattfor on 8/16/26.
//

#ifndef ABBA_WINDOWSMEMORY_H
#define ABBA_WINDOWSMEMORY_H

#if defined(_WIN32)

#include "MemoryMap.h"

/// Windows!
class WindowsMemory final : public MemoryMap
{
public:
    WindowsMemory() = default;

    [[nodiscard]] static memmap::Protection translateProtection(std::uint32_t flags) noexcept;
    [[nodiscard]] static memmap::RegionType translateType(std::uint32_t flags) noexcept;

protected:
    [[nodiscard]] bool collect(std::uint32_t process_id, std::vector<memmap::Region>& regions) const override;
};

#endif

#endif //ABBA_WINDOWSMEMORY_H
