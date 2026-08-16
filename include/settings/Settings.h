//
// Created by mattfor on 8/11/26.
//

#ifndef ABBA_SETTINGS_H
#define ABBA_SETTINGS_H

#include <vector>
#include <filesystem>

#include "../../lib/inicpp/inicpp.hpp"

#include "../utilities/Logger.h"

class Settings
{
    /**
     * Ini file loader holder.
     *
     * @link {https://github.com/dujingning/inicpp}
     */
    struct Loader
    {
        std::filesystem::path               path;
        std::unique_ptr<inicpp::IniManager> manager;
    };

    std::vector<Loader> loaders{};

public:
    Settings() = default;

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
