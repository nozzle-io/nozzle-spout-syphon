#include <app/app_config.hpp>

#include <limits>
#include <sstream>
#include <cstdlib>

namespace nozzle_spout_syphon {

namespace {

bool parse_u32(const std::string &value, std::uint32_t *out) noexcept {
    if (!out || value.empty()) {
        return false;
    }
    char *end = nullptr;
    const unsigned long parsed = std::strtoul(value.c_str(), &end, 10);
    if (!end || *end != '\0' || parsed > std::numeric_limits<std::uint32_t>::max()) {
        return false;
    }
    *out = static_cast<std::uint32_t>(parsed);
    return true;
}

bool read_value(int argc, char **argv, int *index, std::string *value, std::string *error) {
    if (!index || !value || !error) {
        return false;
    }
    if (*index + 1 >= argc) {
        *error = std::string("missing value for ") + argv[*index];
        return false;
    }
    *index = *index + 1;
    *value = argv[*index];
    return true;
}

} // namespace

const char *direction_name(bridge_direction direction) noexcept {
    switch (direction) {
    case bridge_direction::external_to_nozzle: return "external-to-nozzle";
    case bridge_direction::nozzle_to_external: return "nozzle-to-external";
    }
    return "external-to-nozzle";
}

bool parse_direction(const std::string &value, bridge_direction *direction) noexcept {
    if (!direction) {
        return false;
    }
    if (value == "external-to-nozzle" || value == "syphon-to-nozzle" || value == "spout-to-nozzle") {
        *direction = bridge_direction::external_to_nozzle;
        return true;
    }
    if (value == "nozzle-to-external" || value == "nozzle-to-syphon" || value == "nozzle-to-spout") {
        *direction = bridge_direction::nozzle_to_external;
        return true;
    }
    return false;
}

bool parse_publish_toggle(const std::string &value, bool *enabled) noexcept {
    if (!enabled) {
        return false;
    }
    if (value == "on" || value == "true" || value == "1" || value == "yes") {
        *enabled = true;
        return true;
    }
    if (value == "off" || value == "false" || value == "0" || value == "no") {
        *enabled = false;
        return true;
    }
    return false;
}

std::string usage_text(const char *program_name) {
    const char *name = program_name && program_name[0] != '\0' ? program_name : "nozzle-spout-syphon";
    std::ostringstream out;
    out << "usage: " << name << " [options]\n"
        << "\n"
        << "Options:\n"
        << "  --mode external-to-nozzle|nozzle-to-external\n"
        << "  --source SELECTOR    Syphon/Spout or nozzle sender selector to read\n"
        << "  --target NAME        sender name to publish\n"
        << "  --publish on|off     requested publish state\n"
        << "  --width N            requested width diagnostic, 0 means unknown\n"
        << "  --height N           requested height diagnostic, 0 means unknown\n"
        << "  --run                run the platform bridge instead of printing diagnostics\n"
        << "  --list               list external Syphon/Spout sources and exit\n"
        << "  --frames N           maximum frames to bridge, 0 means run until stopped\n"
        << "  --timeout-ms N       receiver/acquire timeout in milliseconds\n"
        << "  --idle-sleep-ms N    polling sleep while waiting for frames\n"
        << "  --help               print this text\n"
        << "\n"
        << "Source selectors for external-to-nozzle: default, name:<exact-name>, app:<exact-app>, or a bare exact name.\n"
        << "Syphon supports name: and app:. Spout supports name: only; app: is rejected because Spout sender enumeration exposes no app field here.\n"
        << "With --run, missing external sources are polled until --timeout-ms; selected sources are sticky and are not auto-switched.\n"
        << "macOS runs the Syphon Metal bridge. Windows runs the SpoutDX D3D11 bridge, with runtime support still gated on host smoke evidence.\n";
    return out.str();
}

parse_result parse_arguments(int argc, char **argv) {
    parse_result result{};
    for (int i = 1; i < argc; ++i) {
        const std::string arg = argv[i];
        std::string value{};
        if (arg == "--help" || arg == "-h") {
            result.config.show_help = true;
        } else if (arg == "--mode") {
            if (!read_value(argc, argv, &i, &value, &result.error)) {
                result.ok = false;
                return result;
            }
            if (!parse_direction(value, &result.config.direction)) {
                result.ok = false;
                result.error = "unsupported mode: " + value;
                return result;
            }
        } else if (arg == "--source") {
            if (!read_value(argc, argv, &i, &result.config.source_name, &result.error)) {
                result.ok = false;
                return result;
            }
        } else if (arg == "--target") {
            if (!read_value(argc, argv, &i, &result.config.target_sender_name, &result.error)) {
                result.ok = false;
                return result;
            }
        } else if (arg == "--publish") {
            if (!read_value(argc, argv, &i, &value, &result.error)) {
                result.ok = false;
                return result;
            }
            if (!parse_publish_toggle(value, &result.config.publish_enabled)) {
                result.ok = false;
                result.error = "unsupported publish toggle: " + value;
                return result;
            }
        } else if (arg == "--width") {
            if (!read_value(argc, argv, &i, &value, &result.error)) {
                result.ok = false;
                return result;
            }
            if (!parse_u32(value, &result.config.requested_width)) {
                result.ok = false;
                result.error = "invalid width: " + value;
                return result;
            }
        } else if (arg == "--height") {
            if (!read_value(argc, argv, &i, &value, &result.error)) {
                result.ok = false;
                return result;
            }
            if (!parse_u32(value, &result.config.requested_height)) {
                result.ok = false;
                result.error = "invalid height: " + value;
                return result;
            }
        } else if (arg == "--run") {
            result.config.run_bridge = true;
        } else if (arg == "--list") {
            result.config.list_sources = true;
        } else if (arg == "--frames") {
            if (!read_value(argc, argv, &i, &value, &result.error)) {
                result.ok = false;
                return result;
            }
            if (!parse_u32(value, &result.config.frame_limit)) {
                result.ok = false;
                result.error = "invalid frame limit: " + value;
                return result;
            }
        } else if (arg == "--timeout-ms") {
            if (!read_value(argc, argv, &i, &value, &result.error)) {
                result.ok = false;
                return result;
            }
            std::uint32_t parsed_timeout{0};
            if (!parse_u32(value, &parsed_timeout)) {
                result.ok = false;
                result.error = "invalid timeout-ms: " + value;
                return result;
            }
            result.config.timeout_ms = parsed_timeout;
        } else if (arg == "--idle-sleep-ms") {
            if (!read_value(argc, argv, &i, &value, &result.error)) {
                result.ok = false;
                return result;
            }
            if (!parse_u32(value, &result.config.idle_sleep_ms)) {
                result.ok = false;
                result.error = "invalid idle-sleep-ms: " + value;
                return result;
            }
        } else {
            result.ok = false;
            result.error = "unknown argument: " + arg;
            return result;
        }
    }
    return result;
}

} // namespace nozzle_spout_syphon
