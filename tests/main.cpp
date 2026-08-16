//
// Created by mattfor on 8/16/26.
//

#include <print>
#include <cstdlib>
#include <string_view>
#include "helpers/TestRunner.h"

namespace SettingsTests
{
    void load_valid_config();
    void get_loaded_manager();
    void read_value();
    void reject_missing_config();
    void reject_duplicate_config();
    void missing_manager_returns_null();
}
namespace HashingTests {
    void one_byte_correct_hash();
    void dummy_file_correct_hash();
}


int main()
{
    constexpr Test tests_settings[] = {
        { "Settings::loadConfig valid config", SettingsTests::load_valid_config },
        { "Settings::getIniManager loaded config", SettingsTests::get_loaded_manager },
        { "Settings::read value", SettingsTests::read_value },
        { "Settings::reject missing config", SettingsTests::reject_missing_config },
        { "Settings::reject duplicate config", SettingsTests::reject_duplicate_config },
        { "Settings::missing manager returns null", SettingsTests::missing_manager_returns_null },

    };
    constexpr Test tests_hashing[] = {
        {"Hashing::one byte returns correct hash", HashingTests::one_byte_correct_hash},
        {"Hashing::dummy file returns correct hash", HashingTests::dummy_file_correct_hash}
    };

    std::size_t passed = 0;
    std::size_t failed = 0;
    std::size_t total = 0;

    if (runTests(tests_settings, passed, failed,total)) return EXIT_FAILURE;
    if (runTests(tests_hashing, passed, failed,total)) return EXIT_FAILURE;

    std::print("\n {}/{} test passed, {} failed\n", passed, total, failed);

    return EXIT_SUCCESS;
}
