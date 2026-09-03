//
// Created by Grzegorz on 8/19/2026.
//


// It's cross platform!
// TODO: Actually do the linux part


#ifndef ABBA_NETWORKMONITOR_H
#define ABBA_NETWORKMONITOR_H

#include <memory>
#include <string>
#include <functional>

#include "netcap/IPacketValidator.h"


class NetworkMonitor
{
public:
    using InvalidPacketCallback = std::function<void(const netcap::PacketContext&, const std::string& reason)>;

    NetworkMonitor();
    ~NetworkMonitor();
    NetworkMonitor(const NetworkMonitor&)               = delete;
    NetworkMonitor&    operator=(const NetworkMonitor&) = delete;
    // I want to allow it to watch different processes, impl_ is per instance, not per class.
    //NOLINTNEXTLINE - Don't make that static, your linter is lying to you!
    [[nodiscard]] bool attach(std::uint32_t processId);
    void               detach();
    [[nodiscard]] bool isAttached() const;

    void addValidator(std::shared_ptr<netcap::IPacketValidator> validator) const;
    void setInvalidPacketCallback(InvalidPacketCallback callback);

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};
#endif
