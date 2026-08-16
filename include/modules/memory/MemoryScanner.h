//
// Created by mattfor on 8/16/26.
//

#ifndef ABBA_MEMORYSCANNER_H
#define ABBA_MEMORYSCANNER_H

#include <atomic>

#include "settings/Settings.h"

class MemoryScanner
{
    std::uint32_t pid{};

public:
    /**
     * TODO
     *
     * @param process_id
     *
     * @return
     */
    bool attach(std::uint32_t process_id);

    /**
     * TODO
     *
     * @param address
     * @param buffer
     * @param size
     *
     * @return
     */
    bool read(std::uintptr_t address, void* buffer, std::size_t size) const;
};


#endif //ABBA_MEMORYSCANNER_H
