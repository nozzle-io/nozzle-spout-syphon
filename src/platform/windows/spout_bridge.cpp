#include <app/status_report.hpp>

#include <sstream>
#include <string>

#if defined(_WIN32)
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#endif

namespace nozzle_spout_syphon {

namespace {

constexpr const char *spout_upstream_tag = "2.007.017";
constexpr const char *spout_upstream_sha = "f49e2f469f8cb25f559a6eaa61a3f5b8173fc100";
constexpr const char *spout_license = "BSD-2-Clause";
constexpr const char *spout_library_dll_name = "SpoutLibrary.dll";
constexpr const char *spout_factory_symbol = "GetSpout";

struct spout_probe_result {
    bool library_loaded{false};
    bool factory_found{false};
    std::string detail{};
};

#if defined(_WIN32)

std::string windows_error_message(DWORD error_code) {
    if (error_code == ERROR_SUCCESS) {
        return "success";
    }

    char *message = nullptr;
    DWORD size = FormatMessageA(
        FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
        nullptr,
        error_code,
        MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
        reinterpret_cast<LPSTR>(&message),
        0,
        nullptr
    );

    std::string result = "Windows error " + std::to_string(error_code);
    if (size != 0 && message) {
        result += ": ";
        result += message;
        while (!result.empty() && (result.back() == '\n' || result.back() == '\r')) {
            result.pop_back();
        }
    }

    if (message) {
        LocalFree(message);
    }
    return result;
}

spout_probe_result probe_spout_library() {
    spout_probe_result result{};
    HMODULE library = LoadLibraryA(spout_library_dll_name);
    if (!library) {
        result.detail = std::string(spout_library_dll_name) + " not loadable: " + windows_error_message(GetLastError());
        return result;
    }

    result.library_loaded = true;
    FARPROC factory = GetProcAddress(library, spout_factory_symbol);
    if (!factory) {
        result.detail = std::string(spout_library_dll_name) + " loaded but GetSpout export is missing: " + windows_error_message(GetLastError());
        FreeLibrary(library);
        return result;
    }

    result.factory_found = true;
    result.detail = std::string(spout_library_dll_name) + " loaded and GetSpout export was found";
    FreeLibrary(library);
    return result;
}

#else

spout_probe_result probe_spout_library() {
    spout_probe_result result{};
    result.detail = "SpoutLibrary probe is Windows-only";
    return result;
}

#endif

std::string build_windows_spout_status_message(const spout_probe_result &probe) {
    std::ostringstream out;
    out << "Spout runtime research integrated; upstream Spout2 " << spout_upstream_tag
        << " (" << spout_upstream_sha << ", " << spout_license << ") audited. ";

    if (probe.factory_found) {
        out << "SpoutLibrary discovery succeeded, but texture bridging is intentionally not enabled without a pinned SDK integration and host smoke evidence.";
    } else if (probe.library_loaded) {
        out << "SpoutLibrary DLL was found but is not ABI-compatible enough for runtime use: " << probe.detail << ".";
    } else {
        out << "No loadable SpoutLibrary DLL was found; vendored/pinned Spout SDK integration is still required.";
    }

    return out.str();
}

std::string build_windows_spout_error_message(const app_config &config, const spout_probe_result &probe) {
    std::ostringstream out;
    out << "Windows Spout runtime path is not implemented. "
        << "Current code only performs DLL/export discovery and records upstream requirements. ";

    if (config.direction == bridge_direction::external_to_nozzle) {
        out << "Required next step: receive a Spout sender via SpoutDX/SpoutLibrary, prove ID3D11Texture2D lifetime/format/origin, then publish into nozzle without claiming zero-copy unless measured. ";
    } else {
        out << "Required next step: receive a nozzle frame, copy or wrap it into a D3D11 texture accepted by SpoutDX/SpoutLibrary SendTexture, then document copy mode. ";
    }

    out << "Probe: " << probe.detail;
    return out.str();
}

} // namespace

bridge_status query_platform_bridge_status(const app_config &config) {
    const spout_probe_result probe = probe_spout_library();

    bridge_status status{};
    status.platform_name = "Windows";
    status.external_system_name = "Spout";
    status.bridge_available = false;
    status.runtime_supported = false;
    status.width = config.requested_width;
    status.height = config.requested_height;
    status.frames_per_second = 0.0;
    status.status_message = build_windows_spout_status_message(probe);
    status.error_message = build_windows_spout_error_message(config, probe);
    return status;
}

std::vector<std::string> list_platform_sources() {
    return {};
}

bridge_run_result run_platform_bridge(const app_config &) {
    bridge_run_result result{};
    result.error_message = "Spout runtime bridge is not implemented in this build";
    result.exit_code = 3;
    return result;
}

} // namespace nozzle_spout_syphon
