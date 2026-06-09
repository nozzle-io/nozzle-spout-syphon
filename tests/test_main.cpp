#include <app/app_config.hpp>
#include <app/source_selection.hpp>
#include <app/status_report.hpp>

#include <cstdio>
#include <cstdlib>
#include <string>
#include <vector>

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
        "--run",
        "--frames", "12",
        "--timeout-ms", "250",
        "--idle-sleep-ms", "3",
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
    require(parsed.config.run_bridge, "run flag is stored");
    require(parsed.config.frame_limit == 12, "frame limit is stored");
    require(parsed.config.timeout_ms == 250, "timeout is stored");
    require(parsed.config.idle_sleep_ms == 3, "idle sleep is stored");
}

void test_status_report_is_honest_about_platform_runtime_state() {
    nozzle_spout_syphon::app_config config{};
    config.requested_width = 640;
    config.requested_height = 360;
    nozzle_spout_syphon::bridge_status status = nozzle_spout_syphon::query_platform_bridge_status(config);
    const std::string report = nozzle_spout_syphon::build_status_report(config, status);

#if defined(__APPLE__)
    require(status.platform_name == "macOS", "macOS platform is reported");
    require(status.external_system_name == "Syphon", "Syphon target is reported");
    require(status.bridge_available, "macOS Syphon bridge is compiled");
    require(!status.runtime_supported, "macOS runtime support remains unclaimed without smoke evidence");
    require(report.find("Bridge available: yes") != std::string::npos, "report says bridge available");
    require(report.find("Runtime supported: no") != std::string::npos, "report does not claim runtime support");
    require(
        report.find("Syphon Metal runtime bridge is compiled") != std::string::npos,
        "report says the Syphon bridge was compiled"
    );
    require(
        report.find("runtime support remains unclaimed") != std::string::npos ||
        report.find("no default Metal device") != std::string::npos,
        "report avoids claiming host smoke coverage"
    );
#elif defined(_WIN32)
    require(status.platform_name == "Windows", "Windows platform is reported");
    require(status.external_system_name == "Spout", "Spout target is reported");
    require(status.bridge_available, "Windows Spout bridge is compiled");
    require(!status.runtime_supported, "Windows Spout runtime support remains unclaimed");
    require(report.find("Bridge available: yes") != std::string::npos, "report says bridge available");
    require(report.find("Runtime supported: no") != std::string::npos, "report says runtime unsupported without smoke");
    require(report.find("SpoutDX D3D11 bridge is compiled and linked") != std::string::npos, "report says SpoutDX path compiled");
#endif
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

std::vector<nozzle_spout_syphon::external_source_info> make_syphon_selection_fixture() {
    std::vector<nozzle_spout_syphon::external_source_info> sources{};
    nozzle_spout_syphon::external_source_info beta{};
    beta.backend = nozzle_spout_syphon::external_source_backend::syphon;
    beta.display_name = "beta_server";
    beta.server_name = "beta_server";
    beta.app_name = "beta_app";
    beta.platform_index = 0;
    sources.push_back(beta);

    nozzle_spout_syphon::external_source_info alpha{};
    alpha.backend = nozzle_spout_syphon::external_source_backend::syphon;
    alpha.display_name = "alpha_server";
    alpha.server_name = "alpha_server";
    alpha.app_name = "alpha_app";
    alpha.platform_index = 1;
    sources.push_back(alpha);
    return sources;
}

void test_source_selector_parsing_is_explicit() {
    nozzle_spout_syphon::source_selector selector = nozzle_spout_syphon::parse_source_selector("");
    require(selector.kind == nozzle_spout_syphon::source_selector_kind::default_source, "empty selector maps to default");
    selector = nozzle_spout_syphon::parse_source_selector("default");
    require(selector.kind == nozzle_spout_syphon::source_selector_kind::default_source, "default selector parses");
    selector = nozzle_spout_syphon::parse_source_selector("name:camera");
    require(selector.kind == nozzle_spout_syphon::source_selector_kind::name, "name selector parses");
    require(selector.value == "camera", "name selector stores value");
    selector = nozzle_spout_syphon::parse_source_selector("app:host");
    require(selector.kind == nozzle_spout_syphon::source_selector_kind::app, "app selector parses");
    require(selector.value == "host", "app selector stores value");
    selector = nozzle_spout_syphon::parse_source_selector("literal");
    require(selector.kind == nozzle_spout_syphon::source_selector_kind::name, "bare selector maps to name");
    require(selector.value == "literal", "bare selector stores literal value");
    selector = nozzle_spout_syphon::parse_source_selector("id:not-supported");
    require(selector.unsupported, "unknown prefixed selector is unsupported");
    selector = nozzle_spout_syphon::parse_source_selector("name:");
    require(selector.unsupported, "empty name selector is unsupported");
    selector = nozzle_spout_syphon::parse_source_selector("app:");
    require(selector.unsupported, "empty app selector is unsupported");

    const std::vector<nozzle_spout_syphon::external_source_info> sources = make_syphon_selection_fixture();
    const nozzle_spout_syphon::source_selection_result result = nozzle_spout_syphon::select_external_source(
        nozzle_spout_syphon::external_source_backend::syphon,
        "id:not-supported",
        sources);
    require(result.status == nozzle_spout_syphon::source_selection_status::unsupported_selector, "unknown prefixed selector is rejected by selection");
}

void test_source_selection_exact_matches_and_missing() {
    const std::vector<nozzle_spout_syphon::external_source_info> sources = make_syphon_selection_fixture();
    nozzle_spout_syphon::source_selection_result result = nozzle_spout_syphon::select_external_source(
        nozzle_spout_syphon::external_source_backend::syphon,
        "name:alpha_server",
        sources);
    require(result.status == nozzle_spout_syphon::source_selection_status::selected, "name selector selects exactly one source");
    require(sources[result.selected_index].server_name == "alpha_server", "name selector picked alpha");

    result = nozzle_spout_syphon::select_external_source(
        nozzle_spout_syphon::external_source_backend::syphon,
        "app:beta_app",
        sources);
    require(result.status == nozzle_spout_syphon::source_selection_status::selected, "app selector selects exactly one source");
    require(sources[result.selected_index].app_name == "beta_app", "app selector picked beta");

    result = nozzle_spout_syphon::select_external_source(
        nozzle_spout_syphon::external_source_backend::syphon,
        "name:missing",
        sources);
    require(result.status == nozzle_spout_syphon::source_selection_status::not_found, "missing source is reported");

    const std::vector<nozzle_spout_syphon::external_source_info> empty_sources{};
    result = nozzle_spout_syphon::select_external_source(
        nozzle_spout_syphon::external_source_backend::syphon,
        "default",
        empty_sources);
    require(result.status == nozzle_spout_syphon::source_selection_status::not_found, "default with no visible sources is not found");
}

void test_source_selection_rejects_duplicate_names_and_apps() {
    std::vector<nozzle_spout_syphon::external_source_info> sources = make_syphon_selection_fixture();
    nozzle_spout_syphon::external_source_info duplicate_name = sources.front();
    duplicate_name.app_name = "other_app";
    duplicate_name.platform_index = 2;
    sources.push_back(duplicate_name);

    nozzle_spout_syphon::source_selection_result result = nozzle_spout_syphon::select_external_source(
        nozzle_spout_syphon::external_source_backend::syphon,
        "name:beta_server",
        sources);
    require(result.status == nozzle_spout_syphon::source_selection_status::ambiguous, "duplicate name is ambiguous");
    require(result.matching_indices.size() == 2, "duplicate name reports both matches");

    sources = make_syphon_selection_fixture();
    nozzle_spout_syphon::external_source_info duplicate_app = sources.back();
    duplicate_app.server_name = "other_server";
    duplicate_app.display_name = "other_server";
    duplicate_app.platform_index = 2;
    sources.push_back(duplicate_app);
    result = nozzle_spout_syphon::select_external_source(
        nozzle_spout_syphon::external_source_backend::syphon,
        "app:alpha_app",
        sources);
    require(result.status == nozzle_spout_syphon::source_selection_status::ambiguous, "duplicate app is ambiguous");
    require(result.matching_indices.size() == 2, "duplicate app reports both matches");

    const std::string list_line = nozzle_spout_syphon::format_external_source_list_line(sources.back(), sources);
    require(list_line.find("selector=\"name:other_server\"") != std::string::npos, "list line contains name selector");
    require(list_line.find("selector=\"app:alpha_app\"") != std::string::npos, "list line contains app selector");
    require(list_line.find("app_ambiguous=yes") != std::string::npos, "list line marks duplicate app ambiguity");
}

void test_default_selection_is_sorted_and_spout_app_is_unsupported() {
    const std::vector<nozzle_spout_syphon::external_source_info> sources = make_syphon_selection_fixture();
    nozzle_spout_syphon::source_selection_result result = nozzle_spout_syphon::select_external_source(
        nozzle_spout_syphon::external_source_backend::syphon,
        "default",
        sources);
    require(result.status == nozzle_spout_syphon::source_selection_status::selected, "default selects when sources exist");
    require(sources[result.selected_index].server_name == "alpha_server", "default picks sorted first source");

    std::vector<nozzle_spout_syphon::external_source_info> spout_sources{};
    nozzle_spout_syphon::external_source_info spout{};
    spout.backend = nozzle_spout_syphon::external_source_backend::spout;
    spout.display_name = "spout_sender";
    spout.server_name = "spout_sender";
    spout_sources.push_back(spout);
    result = nozzle_spout_syphon::select_external_source(
        nozzle_spout_syphon::external_source_backend::spout,
        "app:host",
        spout_sources);
    require(result.status == nozzle_spout_syphon::source_selection_status::unsupported_selector, "Spout app selector is unsupported");
}

void test_list_output_quotes_selector_evidence() {
    std::vector<nozzle_spout_syphon::external_source_info> sources{};
    nozzle_spout_syphon::external_source_info source{};
    source.backend = nozzle_spout_syphon::external_source_backend::syphon;
    source.display_name = "presentation only";
    source.server_name = "Main Output=1";
    source.app_name = "Some \"Host\" App";
    source.platform_index = 7;
    sources.push_back(source);

    const std::string list_line = nozzle_spout_syphon::format_external_source_list_line(source, sources);
    require(list_line.find("name=\"Main Output=1\"") != std::string::npos, "list quotes names with spaces and equals");
    require(list_line.find("app=\"Some \\\"Host\\\" App\"") != std::string::npos, "list escapes quotes in app names");
    require(list_line.find("selector=\"name:Main Output=1\"") != std::string::npos, "list quotes name selector");
    require(list_line.find("selector=\"app:Some \\\"Host\\\" App\"") != std::string::npos, "list quotes app selector");

    nozzle_spout_syphon::source_selection_result result = nozzle_spout_syphon::select_external_source(
        nozzle_spout_syphon::external_source_backend::syphon,
        "name:presentation only",
        sources);
    require(result.status == nozzle_spout_syphon::source_selection_status::not_found, "display_name is not a selector");
}

} // namespace

int main() {
    test_direction_parsing_accepts_canonical_modes();
    test_publish_toggle_parsing_is_explicit();
    test_argument_parser_fills_product_controls();
    test_status_report_is_honest_about_platform_runtime_state();
    test_invalid_arguments_fail_without_fallback_guessing();
    test_source_selector_parsing_is_explicit();
    test_source_selection_exact_matches_and_missing();
    test_source_selection_rejects_duplicate_names_and_apps();
    test_default_selection_is_sorted_and_spout_app_is_unsupported();
    test_list_output_quotes_selector_evidence();
    return 0;
}
