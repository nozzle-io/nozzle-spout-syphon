#include <app/status_report.hpp>

namespace nozzle_spout_syphon {

bridge_status query_platform_bridge_status(const app_config &config) {
    bridge_status status{};
    status.platform_name = "Windows";
    status.external_system_name = "Spout";
    status.bridge_available = false;
    status.runtime_supported = false;
    status.width = config.requested_width;
    status.height = config.requested_height;
    status.frames_per_second = 0.0;
    status.status_message = "Spout bridge scaffold only";
    status.error_message = "Spout SDK integration is not implemented; no runtime texture bridge is available";
    return status;
}

} // namespace nozzle_spout_syphon
