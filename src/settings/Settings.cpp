//
// Created by mattfor on 8/11/26.
//

#include "../../include/settings/Settings.h"

#include "../../include/utilities/Logger.h"
#include "../../include/utilities/VectorHelpers.h"

/**
 * Construct settings validator?
 * Validate keys after loading according to template?
 */

/**
 * placeholder
 */
bool Settings::loadConfig(const std::filesystem::path& configPath)
{
    const std::filesystem::path absolutePath = std::filesystem::absolute(configPath);
    p("Attempting to load .ini config for {}", absolutePath.string());

    if (!std::filesystem::exists(absolutePath))
    {
        p("[ERROR] Config file does not exist: {}", absolutePath.string());
        return false;
    }

    if (!std::filesystem::is_regular_file(absolutePath))
    {
        p("[ERROR] Config path is not a regular file: {}", absolutePath.string());
        return false;
    }

    // Guards
    if (configPath.empty())
    {
        // TODO: log err here
        return false;
    }

    const auto alreadyLoaded = std::ranges::find_if(this->loaders, [&](const Loader& config)
    {
        return config.path == configPath;
    });

    if (alreadyLoaded != loaders.end())
    {
        p("[WARN] .ini config {} has already been loaded!", absolutePath.string());
        return false;
    }

    loaders.emplace_back(Loader{ configPath, std::make_unique<inicpp::IniManager>(configPath.string()) });

    p("[SUCCESS] .ini config {} loaded!", absolutePath.string());

    return true;
}

inicpp::IniManager* Settings::getIniManager(const std::filesystem::path& configPath)
{
    const std::filesystem::path absolutePath = std::filesystem::absolute(configPath);
    p("Accessing .ini config for {}", absolutePath.string());

    if (configPath.empty())
    {
        p("[ERROR] Config path is empty!");
        return nullptr;
    }

    const auto loader = std::ranges::find_if(this->loaders, [&](const Loader& config)
    {
        return config.path == configPath;
    });

    if (loader == this->loaders.end())
    {
        p("[ERROR] .ini config {} has not been loaded!", absolutePath.string());
        return nullptr;
    }

    return loader->manager.get();
}
