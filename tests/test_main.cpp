#include <app/app_config.hpp>
#include <app/status_report.hpp>

#include <cstdio>
#include <cstdlib>
#include <string>

namespace {

void require(bool condition, const char *message) {
    if (!condition) {
        std::fprintf(stderr, "test failure: %s\n", message);
        std::abort();
    }
}

void test_direction_parsing_accepts_canonical_modes() {
    nozzle_spout_syphon::bridge_direction direction = nozzle_spout_syphon::bridge_direction::external_to_nozzle;
    require(nozzle_spout_syphon::parse_direction("external-to-nozzle", &direction), "external-to-nozzle parses");
    require(
        direction == nozzle_spout_syphon::bridge_direction::external_to_nozzle,
        "external-to-nozzle maps to external_to_nozzle"
    );
    require(nozzle_spout_syphon::parse_direction("nozzle-to-external", &direction), "nozzle-to-external parses");
    require(
        direction == nozzle_spout_syphon::bridge_direction::nozzle_to_external,
        "nozzle-to-external maps to nozzle_to_external"
    );
    require(!nozzle_spout_syphon::parse_direction("linux-to-nozzle", &direction), "unsupported direction is rejected");
}

void test_publish_toggle_parsing_is_explicit() {
    bool enabled = false;
    require(nozzle_spout_syphon::parse_publish_toggle("on", &enabled), "on parses");
    require(enabled, "on enables publish");
    require(nozzle_spout_syphon::parse_publish_toggle("off", &enabled), "off parses");
    require(!enabled, "off disables publish");
    require(!nozzle_spout_syphon::parse_publish_toggle("maybe", &enabled), "ambiguous publish toggle is rejected");
}

void test_argument_parser_fills_product_controls() {
    const char *args[] = {
        "nozzle-spout-syphon",
        "--mode", "nozzle-to-external",
        "--source", "nozzle_viewer",
        "--target", "syphon_out",
        "--publish", "off",
        "--width", "1280",
        "--height", "720",
    };
    auto *argv = const_cast<char **>(args);
    nozzle_spout_syphon::parse_result parsed = nozzle_spout_syphon::parse_arguments(
        static_cast<int>(sizeof(args) / sizeof(args[0])),
        argv
    );
    require(parsed.ok, "full argument set parses");
    require(parsed.config.direction == nozzle_spout_syphon::bridge_direction::nozzle_to_external, "mode is stored");
    require(parsed.config.source_name == "nozzle_viewer", "source is stored");
    require(parsed.config.target_sender_name == "syphon_out", "target is stored");
    require(!parsed.config.publish_enabled, "publish toggle is stored");
    require(parsed.config.requested_width == 1280, "width is stored");
    require(parsed.config.requested_height == 720, "height is stored");
}

void test_status_report_is_honest_about_unavailable_runtime() {
    nozzle_spout_syphon::app_config config{};
    config.requested_width = 640;
    config.requested_height = 360;
    nozzle_spout_syphon::bridge_status status = nozzle_spout_syphon::query_platform_bridge_status(config);
    require(!status.bridge_available, "bridge is unavailable in scaffold");
    require(!status.runtime_supported, "runtime is unsupported in scaffold");
    const std::string report = nozzle_spout_syphon::build_status_report(config, status);
    require(report.find("Bridge available: no") != std::string::npos, "report says bridge unavailable");
    require(report.find("Runtime supported: no") != std::string::npos, "report says runtime unsupported");
    require(report.find("not runtime-validated") != std::string::npos, "report avoids runtime format validation claim");
}

void test_invalid_arguments_fail_without_fallback_guessing() {
    const char *args[] = {"nozzle-spout-syphon", "--mode", "bad"};
    auto *argv = const_cast<char **>(args);
    nozzle_spout_syphon::parse_result parsed = nozzle_spout_syphon::parse_arguments(
        static_cast<int>(sizeof(args) / sizeof(args[0])),
        argv
    );
    require(!parsed.ok, "bad mode fails");
    require(parsed.error.find("unsupported mode") != std::string::npos, "bad mode reports parse error");
}

} // namespace

int main() {
    test_direction_parsing_accepts_canonical_modes();
    test_publish_toggle_parsing_is_explicit();
    test_argument_parser_fills_product_controls();
    test_status_report_is_honest_about_unavailable_runtime();
    test_invalid_arguments_fail_without_fallback_guessing();
    return 0;
}
