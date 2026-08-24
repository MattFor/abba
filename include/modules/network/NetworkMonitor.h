//
// Created by Grzegorz on 8/19/2026.
//

#pragma once

#if defined(_WIN32)

#ifndef ABBA_NETWORKMONITOR_H
#define ABBA_NETWORKMONITOR_H

#include <memory>
#include <string>
#include <vector>
#include <optional>
#include <functional>

#include <bemapiset.h>


#include "netcap/IPacketValidator.h"

struct ImportedFunction
{
    std::string dllName{};
    std::string functionName{};
};std::optional<std::vector<ImportedFunction>> listImports(HMODULE module);


/// Windows!
class NetworkMonitor
{
public:
    using InvalidPacketCallback = std::function<void(const netcap::PacketContext &, const std::string & reason)>;

    NetworkMonitor();
    ~NetworkMonitor();
    NetworkMonitor(const NetworkMonitor&)            = delete;
    NetworkMonitor& operator=(const NetworkMonitor&) = delete;

    [[nodiscard]] static bool attach(std::uint32_t processId);
    void                      detach();
    [[nodiscard]] bool        isAttached() const;

    void addValidator(std::shared_ptr<netcap::IPacketValidator> validator) const;
    void setInvalidPacketCallback(InvalidPacketCallback callback);

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};
#endif
#endif
