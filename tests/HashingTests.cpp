//
// Created by Grzegorz on 8/16/2026.
//

#include "helpers/TempFile.h"
#include "utilities/Hashing.h"
#include <stdexcept>

namespace HashingTests
{
    void one_byte_correct_hash()
    {
        if (Hashing::hashFromData({
                std::byte{
                    0x01
                }
            }) != "4bf5122f344554c53bde2ebb8cd2b7e3d1600ad631c385a5d7cce23c7785459a")
        {
            throw std::runtime_error("one byte returns correct hash: hash mismatch");
        }
    }

    void dummy_file_correct_hash()
    {
        const TempFile file({
            0x00,
            0x01,
            0x02,
            0x03,
            0x04
        });

        if (const auto hash = Hashing::hashFromFile(file.path()); !hash || *hash != "08bb5e5d6eaac1049ede0893d30ed022b1a4d9b5b48db414871f51c9cb35283d")
        {
            throw std::runtime_error("dummy file returns correct hash: hash mismatch");
        }
    }
}
