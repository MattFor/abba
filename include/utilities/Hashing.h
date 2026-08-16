//
// Created by Grzegorz on 8/16/2026.
//

#ifndef ABBA_HASHING_H
#define ABBA_HASHING_H
#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <vector>
#include <filesystem>
#include "FileOperations.h"



class Hashing {
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
    [[nodiscard]] static std::optional<std::string> hashFromFile(const FileOperations::FileInfo& file );
    [[nodiscard]] static std::optional<std::string> hashFromFile(const std::filesystem::path &path);
};


#endif //ABBA_HASHING_H