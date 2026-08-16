#include "include/utilities/Logger.h"
#include "include/settings/Settings.h"

int main()
{
    p("Starting program...");

    Settings settings{};

    if (!settings.loadConfig("./config/program/dev.ini"))
    {
        p("[ERROR] Failed to obtain development config!");
        return 1;
    }

    auto* dev_ini_manager = settings.getIniManager("./config/program/dev.ini");

    const auto value = ( *dev_ini_manager )["test"]["test"].get<std::string>();

    std::print("The value of [test][test] is {}\n", value);

    // 1. Load settings

    // 2. Attach to binary

    // 3. Create scanner loops
}
