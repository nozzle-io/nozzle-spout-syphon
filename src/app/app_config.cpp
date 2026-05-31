#include <app/app_config.hpp>

#include <cstdlib>
#include <limits>
#include <sstream>

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
        << "  --source NAME        Syphon/Spout or nozzle sender name to read\n"
        << "  --target NAME        sender name to publish\n"
        << "  --publish on|off     requested publish state\n"
        << "  --width N            requested width diagnostic, 0 means unknown\n"
        << "  --height N           requested height diagnostic, 0 means unknown\n"
        << "  --help               print this text\n"
        << "\n"
        << "This scaffold reports bridge availability only. It does not perform runtime texture bridging.\n";
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
        } else {
            result.ok = false;
            result.error = "unknown argument: " + arg;
            return result;
        }
    }
    return result;
}

} // namespace nozzle_spout_syphon
