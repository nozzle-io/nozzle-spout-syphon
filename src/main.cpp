#include <app/app_config.hpp>
#include <app/status_report.hpp>

#include <cstdio>
#include <string>
#include <vector>

int main(int argc, char **argv) {
    nozzle_spout_syphon::parse_result parsed = nozzle_spout_syphon::parse_arguments(argc, argv);
    if (!parsed.ok) {
        std::fprintf(stderr, "%s\n\n%s", parsed.error.c_str(), nozzle_spout_syphon::usage_text(argv[0]).c_str());
        return 2;
    }

    if (parsed.config.show_help) {
        std::printf("%s", nozzle_spout_syphon::usage_text(argv[0]).c_str());
        return 0;
    }

    if (parsed.config.list_sources) {
        const std::vector<std::string> sources = nozzle_spout_syphon::list_platform_sources();
        for (const std::string &source : sources) {
            std::printf("%s\n", source.c_str());
        }
        return 0;
    }

    if (parsed.config.run_bridge) {
        if (!parsed.config.publish_enabled) {
            std::fprintf(stderr, "--publish off cannot be used with --run; runtime bridge output would be disabled\n");
            return 2;
        }
        const nozzle_spout_syphon::bridge_run_result result =
            nozzle_spout_syphon::run_platform_bridge(parsed.config);
        if (!result.status_message.empty()) {
            std::printf("%s\n", result.status_message.c_str());
        }
        if (!result.error_message.empty()) {
            std::fprintf(stderr, "%s\n", result.error_message.c_str());
        }
        std::printf("Frames processed: %llu\n", static_cast<unsigned long long>(result.frames_processed));
        return result.exit_code;
    }

    const nozzle_spout_syphon::bridge_status status = nozzle_spout_syphon::query_platform_bridge_status(parsed.config);
    const std::string report = nozzle_spout_syphon::build_status_report(parsed.config, status);
    std::printf("%s", report.c_str());
    return 0;
}
