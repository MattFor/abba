//
// Created by Grzegorz on 8/19/2026.
//

#ifndef ABBA_INJECTOR_H
#define ABBA_INJECTOR_H
#include <cstdint>
#include <filesystem>


class Injector
{
public:
    [[nodiscard]] bool inject(std::uint32_t pid, const std::filesystem::path& dllPath) const;
};

#endif //ABBA_INJECTOR_H
