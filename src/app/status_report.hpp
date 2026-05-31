#pragma once

#include <app/app_config.hpp>

#include <cstdint>
#include <string>

namespace nozzle_spout_syphon {

struct bridge_status {
    std::string platform_name{};
    std::string external_system_name{};
    bool bridge_available{false};
    bool runtime_supported{false};
    std::uint32_t width{0};
    std::uint32_t height{0};
    double frames_per_second{0.0};
    std::string status_message{};
    std::string error_message{};
};

bridge_status query_platform_bridge_status(const app_config &config);
std::string build_status_report(const app_config &config, const bridge_status &status);
const char *availability_text(bool value) noexcept;

} // namespace nozzle_spout_syphon
