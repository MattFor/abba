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
            "Settings::loadConfig valid config",
            SettingsTests::load_valid_config
        },
        {
            "Settings::getIniManager loaded config",
            SettingsTests::get_loaded_manager
        },
        {
            "Settings::read value",
            SettingsTests::read_value
        },
        {
            "Settings::reject missing config",
            SettingsTests::reject_missing_config
        },
        {
            "Settings::reject duplicate config",
            SettingsTests::reject_duplicate_config
        },
        {
            "Settings::missing manager returns null",
            SettingsTests::missing_manager_returns_null
        },
    };

    constexpr Test tests_hashing[] = {
        {
            "Hashing::one byte returns correct hash",
            HashingTests::one_byte_correct_hash
        },
        {
            "Hashing::dummy file returns correct hash",
            HashingTests::dummy_file_correct_hash
        }
    };

    constexpr Test tests_memory_scanner[] = {
        {
            "MemoryScanner::attach current process",
            MemoryScannerTests::attach_current_process
        },
        {
            "MemoryScanner::attach current process twice",
            MemoryScannerTests::attach_current_process_twice
        },
        {
            "MemoryScanner::read integer",
            MemoryScannerTests::read_integer
        },
        {
            "MemoryScanner::read single byte",
            MemoryScannerTests::read_single_byte
        },
        {
            "MemoryScanner::read buffer",
            MemoryScannerTests::read_buffer
        },
        {
            "MemoryScanner::read string",
            MemoryScannerTests::read_string
        },
        {
            "MemoryScanner::read struct",
            MemoryScannerTests::read_struct
        },
        {
            "MemoryScanner::read does not overwrite beyond size",
            MemoryScannerTests::read_does_not_overwrite_beyond_size
        },
        {
            "MemoryScanner::read invalid address",
            MemoryScannerTests::reject_invalid_address
        },
        {
            "MemoryScanner::read null buffer",
            MemoryScannerTests::reject_null_buffer
        },
        {
            "MemoryScanner::read zero bytes",
            MemoryScannerTests::read_zero_bytes
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
