//
// Created by mattfor on 8/16/26.
//

#include <print>
#include <cstdlib>
#include <string_view>

#include "helpers/TestRunner.h"

namespace SettingsTests
{
    void read_value();
    void load_valid_config();
    void get_loaded_manager();
    void reject_missing_config();
    void reject_duplicate_config();
    void missing_manager_returns_null();
}

namespace HashingTests
{
    void one_byte_correct_hash();
    void dummy_file_correct_hash();
}

namespace MemoryScannerTests
{
    void read_buffer();
    void read_string();
    void read_struct();
    void read_integer();
    void read_zero_bytes();
    void read_single_byte();
    void reject_null_buffer();
    void attach_current_process();
    void reject_invalid_address();
    void attach_current_process_twice();
    void read_does_not_overwrite_beyond_size();
}

int main()
{
    constexpr Test tests_settings[] = {
        {
            .name = "Settings::loadConfig valid config",
            .function = SettingsTests::load_valid_config
        },
        {
            .name = "Settings::getIniManager loaded config",
            .function = SettingsTests::get_loaded_manager
        },
        {
            .name = "Settings::read value",
            .function = SettingsTests::read_value
        },
        {
            .name = "Settings::reject missing config",
            .function = SettingsTests::reject_missing_config
        },
        {
            .name = "Settings::reject duplicate config",
            .function = SettingsTests::reject_duplicate_config
        },
        {
            .name = "Settings::missing manager returns null",
            .function = SettingsTests::missing_manager_returns_null
        },
    };

    constexpr Test tests_hashing[] = {
        {
            .name = "Hashing::one byte returns correct hash",
            .function = HashingTests::one_byte_correct_hash
        },
        {
            .name = "Hashing::dummy file returns correct hash",
            .function = HashingTests::dummy_file_correct_hash
        }
    };

    constexpr Test tests_memory_scanner[] = {
        {
            .name = "MemoryScanner::attach current process",
            .function = MemoryScannerTests::attach_current_process
        },
        {
            .name = "MemoryScanner::attach current process twice",
            .function = MemoryScannerTests::attach_current_process_twice
        },
        {
            .name = "MemoryScanner::read integer",
            .function = MemoryScannerTests::read_integer
        },
        {
            .name = "MemoryScanner::read single byte",
            .function = MemoryScannerTests::read_single_byte
        },
        {
            .name = "MemoryScanner::read buffer",
            .function = MemoryScannerTests::read_buffer
        },
        {
            .name = "MemoryScanner::read string",
            .function = MemoryScannerTests::read_string
        },
        {
            .name = "MemoryScanner::read struct",
            .function = MemoryScannerTests::read_struct
        },
        {
            .name = "MemoryScanner::read does not overwrite beyond size",
            .function = MemoryScannerTests::read_does_not_overwrite_beyond_size
        },
        {
            .name = "MemoryScanner::read invalid address",
            .function = MemoryScannerTests::reject_invalid_address
        },
        {
            .name = "MemoryScanner::read null buffer",
            .function = MemoryScannerTests::reject_null_buffer
        },
        {
            .name = "MemoryScanner::read zero bytes",
            .function = MemoryScannerTests::read_zero_bytes
        }
    };

    std::size_t total  = 0;
    std::size_t passed = 0;
    std::size_t failed = 0;

    std::size_t current = 0;

    if (runTests(tests_settings, passed, failed, total, current))
    {
        return EXIT_FAILURE;
    }

    if (runTests(tests_hashing, passed, failed, total, current))
    {
        return EXIT_FAILURE;
    }

    if (runTests(tests_memory_scanner, passed, failed, total, current))
    {
        return EXIT_FAILURE;
    }

    std::print("\n {}/{} test passed, {} failed\n", passed, total, failed);

    return EXIT_SUCCESS;
}
