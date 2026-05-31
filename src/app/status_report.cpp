#include <app/status_report.hpp>

#include <nozzle/types.hpp>

#include <sstream>

namespace nozzle_spout_syphon {

namespace {

const char *format_name(nozzle::texture_format format) noexcept {
    switch (format) {
    case nozzle::texture_format::rgba8_unorm: return "rgba8_unorm";
    case nozzle::texture_format::bgra8_unorm: return "bgra8_unorm";
    case nozzle::texture_format::unknown: return "unknown";
    default: return "not-claimed";
    }
}

} // namespace

const char *availability_text(bool value) noexcept {
    return value ? "yes" : "no";
}

std::string build_status_report(const app_config &config, const bridge_status &status) {
    std::ostringstream out;
    out << "nozzle-spout-syphon " << NOZZLE_SPOUT_SYPHON_VERSION << '\n'
        << "Platform: " << status.platform_name << '\n'
        << "External system: " << status.external_system_name << '\n'
        << "Mode: " << direction_name(config.direction) << '\n'
        << "Source: " << config.source_name << '\n'
        << "Target sender: " << config.target_sender_name << '\n'
        << "Publish requested: " << availability_text(config.publish_enabled) << '\n'
        << "Requested resolution: " << config.requested_width << "x" << config.requested_height << '\n'
        << "Bridge available: " << availability_text(status.bridge_available) << '\n'
        << "Runtime supported: " << availability_text(status.runtime_supported) << '\n'
        << "Observed resolution: " << status.width << "x" << status.height << '\n'
        << "Observed fps: " << status.frames_per_second << '\n'
        << "Reserved texture format scope: " << format_name(nozzle::texture_format::rgba8_unorm) << " (not runtime-validated)" << '\n'
        << "Status: " << status.status_message << '\n';
    if (!status.error_message.empty()) {
        out << "Error: " << status.error_message << '\n';
    }
    return out.str();
}

} // namespace nozzle_spout_syphon
