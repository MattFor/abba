//
// Created by mattfor on 8/11/26.
//

#ifndef ABBA_SETTINGS_H
#define ABBA_SETTINGS_H

#include <vector>
#include <filesystem>

#include <inicpp/inicpp.hpp>

#include "../utilities/Logger.h"

// Get the platform early
enum class Platform
{
    Linux,
    Windows
};

constexpr Platform getPlatform()
{
#if defined(_WIN32)
    return Platform::Windows;
#elif defined(__linux__)
    return Platform::Linux;
#else
#error "Unsupported platform"
#endif
}

inline static constexpr Platform platform = getPlatform();

class Settings
{
    Settings() = default;

    /**
     * Ini file loader holder.
     *
     * @see https://github.com/dujingning/inicpp
     */
    struct Loader
    {
        std::filesystem::path               path;
        std::unique_ptr<inicpp::IniManager> manager;
    };

    std::vector<Loader> loaders{};

public:
    // Make it a singleton
    static Settings& instance()
    {
        static Settings settings;
        return settings;
    }

    /**
     * Load an .ini file config into memory.
     *
     * @param configPath Path to the .ini config file.
     *
     * @returns TODO
     */
    bool loadConfig(const std::filesystem::path& configPath);

    /**
     * TODO
     * @param configPath
     * @return
     */
    inicpp::IniManager* getIniManager(const std::filesystem::path& configPath);
};


#endif //ABBA_SETTINGS_H
