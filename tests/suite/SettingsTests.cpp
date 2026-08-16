//
// Created by mattfor on 8/16/26.
//

#include "settings/Settings.h"

#include <string>
#include <stdexcept>

namespace SettingsTests
{
    void load_valid_config()
    {
        if (auto& settings = Settings::instance(); !settings.loadConfig("tests/config/test.ini"))
        {
            throw std::runtime_error("load_valid_config: loadConfig() returned false");
        }
    }

    void get_loaded_manager()
    {
        auto& settings = Settings::instance();

        const auto path = "tests/config/test.ini";

        if (const auto* manager = settings.getIniManager(path); manager == nullptr)
        {
            throw std::runtime_error("get_loaded_manager: getIniManager() returned nullptr");
        }
    }

    void read_value()
    {
        auto& settings = Settings::instance();

        const auto path = "tests/config/test.ini";

        auto* manager = settings.getIniManager(path);

        if (manager == nullptr)
        {
            throw std::runtime_error("read_value: getIniManager() returned nullptr");
        }

        if (const auto value = ( *manager )["test"]["test"].get<std::string>(); value != "true")
        {
            throw std::runtime_error("read_value: expected 'true', got '" + value + "'");
        }
    }

    void reject_missing_config()
    {
        if (auto& settings = Settings::instance(); settings.loadConfig("config/does_not_exist.ini"))
        {
            throw std::runtime_error("reject_missing_config: missing config was accepted");
        }
    }

    void reject_duplicate_config()
    {
        auto& settings = Settings::instance();

        const auto path = "tests/config/duplicate_test.ini";

        if (!settings.loadConfig(path))
        {
            throw std::runtime_error("reject_duplicate_config: first load failed");
        }

        if (settings.loadConfig(path))
        {
            throw std::runtime_error("reject_duplicate_config: duplicate load was accepted");
        }
    }

    void missing_manager_returns_null()
    {
        auto& settings = Settings::instance();

        if (const auto* manager = settings.getIniManager("tests/config/never_loaded.ini"); manager != nullptr)
        {
            throw std::runtime_error("missing_manager_returns_null: expected nullptr");
        }
    }
}
