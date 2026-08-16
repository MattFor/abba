//
// Created by mattfor on 8/16/26.
//

#include <print>
#include <cstdlib>
#include <string_view>

namespace SettingsTests
{
    void load_valid_config();
    void get_loaded_manager();
    void read_value();
    void reject_missing_config();
    void reject_duplicate_config();
    void missing_manager_returns_null();
}

int main()
{
    struct Test
    {
        std::string_view name;
        void (*          function)();
    };

    constexpr Test tests[] = { { "Settings::loadConfig valid config", SettingsTests::load_valid_config }, { "Settings::getIniManager loaded config", SettingsTests::get_loaded_manager }, { "Settings::read value", SettingsTests::read_value }, { "Settings::reject missing config", SettingsTests::reject_missing_config }, { "Settings::reject duplicate config", SettingsTests::reject_duplicate_config }, { "Settings::missing manager returns null", SettingsTests::missing_manager_returns_null }, };

    std::size_t passed = 0;
    std::size_t failed = 0;

    for (const auto& [name, function] : tests)
    {
        try
        {
            function();

            std::print("[PASS] {}\n", name);
            ++passed;
        }
        catch (const std::exception& e)
        {
            std::print("[FAIL] {}\n", e.what());

            ++failed;

            return EXIT_FAILURE;
        }
        catch (...)
        {
            std::print("[FAIL] Unknown exception\n");

            ++failed;

            return EXIT_FAILURE;
        }
    }

    std::print("\n {}/{} test passed, {} failed\n", passed, std::size(tests), failed);

    return EXIT_SUCCESS;
}
