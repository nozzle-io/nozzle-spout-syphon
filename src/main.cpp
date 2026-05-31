#include <app/app_config.hpp>
#include <app/status_report.hpp>

#include <cstdio>
#include <string>

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

    const nozzle_spout_syphon::bridge_status status = nozzle_spout_syphon::query_platform_bridge_status(parsed.config);
    const std::string report = nozzle_spout_syphon::build_status_report(parsed.config, status);
    std::printf("%s", report.c_str());
    return 0;
}
