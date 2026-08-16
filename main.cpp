#include "include/utilities/Logger.h"
#include "include/settings/Settings.h"

int main()
{
    p("Starting program...");
    //NOLINTNEXTLINE
    p("Current platform: {}", platform == Platform::Linux ? "Linux" : "Windows");

    // 1. Load settings
    auto& settings = Settings::instance();

    if (!settings.loadConfig("./config/program/settings.ini"))
    {
        p("[ERROR] Failed to obtain development config!");
        return EXIT_FAILURE;
    }

    auto* dev_ini_manager = settings.getIniManager("./config/program/settings.ini");

    p("Checking memory scanner module settings...");
    const auto memory_scanner_on = ( *dev_ini_manager )["memory"]["on"].get<bool>();
    p("{}Memory scanner is {}\n", memory_scanner_on ? "[SUCCESS] " : "", memory_scanner_on ? "ON" : "OFF");

    // 2. Attach to binary

    // 3. Create scanner loops
}
