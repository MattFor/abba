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
        if (Settings settings; !settings.loadConfig("config/program/dev.ini"))
        {
            throw std::runtime_error("load_valid_config: loadConfig() returned false");
        }
    }

    void get_loaded_manager()
    {
        Settings settings;

        const auto path = "config/program/dev.ini";

        if (!settings.loadConfig(path))
        {
            throw std::runtime_error("get_loaded_manager: failed to load config");
        }

        if (const auto* manager = settings.getIniManager(path); manager == nullptr)
        {
            throw std::runtime_error("get_loaded_manager: getIniManager() returned nullptr");
        }
    }

    void read_value()
    {
        Settings settings;

        const auto path = "config/program/dev.ini";

        if (!settings.loadConfig(path))
        {
            throw std::runtime_error("read_value: failed to load config");
        }

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
        if (Settings settings; settings.loadConfig("config/does_not_exist.ini"))
        {
            throw std::runtime_error("reject_missing_config: missing config was accepted");
        }
    }

    void reject_duplicate_config()
    {
        Settings settings;

        const auto path = "config/program/dev.ini";

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
        Settings settings;

        if (const auto* manager = settings.getIniManager("config/program/dev.ini"); manager != nullptr)
        {
            throw std::runtime_error("missing_manager_returns_null: expected nullptr");
        }
    }
}
