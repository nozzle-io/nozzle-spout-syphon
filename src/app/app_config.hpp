#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace nozzle_spout_syphon {

enum class bridge_direction {
    external_to_nozzle,
    nozzle_to_external,
};

struct app_config {
    bridge_direction direction{bridge_direction::external_to_nozzle};
    std::string source_name{"default"};
    std::string target_sender_name{"nozzle-spout-syphon"};
    bool publish_enabled{true};
    std::uint32_t requested_width{0};
    std::uint32_t requested_height{0};
    std::uint32_t frame_limit{0};
    std::uint64_t timeout_ms{1000};
    std::uint32_t idle_sleep_ms{10};
    bool run_bridge{false};
    bool list_sources{false};
    bool show_help{false};
};

struct parse_result {
    app_config config{};
    bool ok{true};
    std::string error{};
};

parse_result parse_arguments(int argc, char **argv);
const char *direction_name(bridge_direction direction) noexcept;
bool parse_direction(const std::string &value, bridge_direction *direction) noexcept;
bool parse_publish_toggle(const std::string &value, bool *enabled) noexcept;
std::string usage_text(const char *program_name);

} // namespace nozzle_spout_syphon
