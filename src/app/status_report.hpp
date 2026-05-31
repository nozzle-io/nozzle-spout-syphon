#pragma once

#include <app/app_config.hpp>

#include <cstdint>
#include <string>
#include <vector>

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

struct bridge_run_result {
    bool ok{false};
    int exit_code{1};
    std::uint64_t frames_processed{0};
    std::string status_message{};
    std::string error_message{};
};

bridge_status query_platform_bridge_status(const app_config &config);
std::vector<std::string> list_platform_sources();
bridge_run_result run_platform_bridge(const app_config &config);
std::string build_status_report(const app_config &config, const bridge_status &status);
const char *availability_text(bool value) noexcept;

} // namespace nozzle_spout_syphon
