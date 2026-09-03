//
// Created by Grzegorz on 8/19/2026.
//

#ifndef ABBA_INJECTOR_H
#define ABBA_INJECTOR_H

#include <cstdint>  // NOTE: unused on linux
#include <filesystem>

/// Windows!
class Injector
{
public:
    [[nodiscard]] static bool inject(std::uint32_t pid, const std::filesystem::path& dllPath);
};

#endif //ABBA_INJECTOR_H
