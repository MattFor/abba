//
// Created by Grzegorz on 8/16/2026.
//

#include "utilities/Hashing.h"
#include "../lib/PicoSHA2/picosha2.h"

std::string Hashing::hashFromData(const std::vector<std::byte>& data)
{
    return picosha2::hash256_hex_string(reinterpret_cast<const unsigned char*>(data.data()), reinterpret_cast<const unsigned char*>(data.data() + data.size()));
}

std::optional<std::string> Hashing::hashFromFile(const std::filesystem::path& path)
{
    const auto bytes = FileOperations::readBinary(path);

    if (!bytes)
    {
        return std::nullopt;
    }

    return hashFromData(*bytes);
}

std::optional<std::string> Hashing::hashFromFile(const FileOperations::FileInfo& file)
{
    const auto bytes = FileOperations::readBinary(file.path);

    if (!bytes)
    {
        return std::nullopt;
    }

    return hashFromData(*bytes);
}
