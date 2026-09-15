//
// Created by mattfor on 8/16/26.
//

#ifndef ABBA_LINUXMEMORY_H
#define ABBA_LINUXMEMORY_H

#if defined( __linux__ )

	#include "MemoryMap.h"

/// Linux!
class LinuxMemory final : public MemoryMap
{
  public:
	LinuxMemory() = default;

	[[nodiscard]] static std::optional<memmap::Region> parseLine( std::string_view line );

  protected:
	[[nodiscard]] bool collect( std::uint32_t process_id, std::vector<memmap::Region>& regions ) const override;
};

#endif

#endif // ABBA_LINUXMEMORY_H
