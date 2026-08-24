#include "include/utilities/Logger.h"
#include "include/settings/Settings.h"
#include "modules/network/NetworkMonitor.h"
#include "modules/network/netcap/PacketValidator.h"
#include "modules/network/netcap/hook/HookProtocol.h"

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

    // Windows
#if defined(_WIN32)
    const std::uint32_t kTempTargetPid = 0; // TODO: Replace with real target binary, from settings, from wherever

    NetworkMonitor networkMonitor;
    networkMonitor.addValidator(std::make_shared<PacketValidator>(PacketValidator::Rules{
        .min_length = 1,
        .max_length = netcap::hookproto::kMaxPayloadBytes,
        .reject_all_zero = true,
        .reject_all_same = true
    }));
    networkMonitor.setInvalidPacketCallback([](const netcap::PacketContext& packet, const std::string& reason)
    {
        p("VIOLATION: pid={} reason={}", packet.processId, reason);
    });

    if (!networkMonitor.attach(kTempTargetPid))
    {
        p("[ERROR] Failed to attach NetworkMonitor");
    }
#endif

    // 3. Create scanner loops
}
