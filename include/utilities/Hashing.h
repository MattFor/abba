//
// Created by Grzegorz on 8/16/2026.
//

#ifndef ABBA_HASHING_H
#define ABBA_HASHING_H

#include <string>
#include <vector>
#include <cstddef>
#include <optional>
#include <filesystem>

#include "FileOperations.h"

class Hashing
{
public:
    /*
     * Create a hash from data in bytes
     *
     * Example: hashFromData({std::byte{0x01}})
     */
    [[nodiscard]] static std::string hashFromData(const std::vector<std::byte>& data);

    /*
     * Create a hash by reading data from path from object
     */
    [[nodiscard]] static std::optional<std::string> hashFromFile(const std::filesystem::path& path);
    [[nodiscard]] static std::optional<std::string> hashFromFile(const FileOperations::FileInfo& file);
};


#endif //ABBA_HASHING_H
