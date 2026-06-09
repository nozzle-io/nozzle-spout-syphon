#include <app/status_report.hpp>
#include <app/source_selection.hpp>

#include <nozzle/backends/d3d11.hpp>
#include <nozzle/receiver.hpp>
#include <nozzle/sender.hpp>

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <d3d11.h>
#include <dxgi.h>
#include <windows.h>

#include <SpoutDX.h>

#include <chrono>
#include <sstream>
#include <string>
#include <thread>
#include <utility>
#include <vector>

namespace nozzle_spout_syphon {

namespace {

constexpr const char *spout_upstream_tag = "2.007.017";
constexpr const char *spout_upstream_sha = "f49e2f469f8cb25f559a6eaa61a3f5b8173fc100";
constexpr const char *spout_license = "BSD-2-Clause";
constexpr const char *spout_sdk_path = "deps/Spout2/SPOUTSDK/SpoutDirectX/SpoutDX";

struct d3d11_device_reference {
    ID3D11Device *device{nullptr};

    ~d3d11_device_reference() {
        reset(nullptr);
    }

    d3d11_device_reference() = default;
    d3d11_device_reference(const d3d11_device_reference &) = delete;
    d3d11_device_reference &operator=(const d3d11_device_reference &) = delete;

    void reset(ID3D11Device *next_device) {
        if (device) {
            device->Release();
        }
        device = next_device;
    }
};

std::chrono::steady_clock::time_point make_deadline(const app_config &config) {
    const std::uint64_t timeout_ms = config.timeout_ms == 0 ? 1000 : config.timeout_ms;
    return std::chrono::steady_clock::now() + std::chrono::milliseconds(timeout_ms);
}

bool deadline_expired(std::chrono::steady_clock::time_point deadline) {
    return deadline <= std::chrono::steady_clock::now();
}

std::chrono::milliseconds idle_sleep(const app_config &config) {
    return std::chrono::milliseconds(config.idle_sleep_ms == 0 ? 1 : config.idle_sleep_ms);
}

nozzle::texture_format format_from_dxgi(DXGI_FORMAT format) {
    switch (format) {
        case DXGI_FORMAT_R8_UNORM: return nozzle::texture_format::r8_unorm;
        case DXGI_FORMAT_R8G8_UNORM: return nozzle::texture_format::rg8_unorm;
        case DXGI_FORMAT_R8G8B8A8_UNORM: return nozzle::texture_format::rgba8_unorm;
        case DXGI_FORMAT_B8G8R8A8_UNORM: return nozzle::texture_format::bgra8_unorm;
        case DXGI_FORMAT_R8G8B8A8_UNORM_SRGB: return nozzle::texture_format::rgba8_srgb;
        case DXGI_FORMAT_B8G8R8A8_UNORM_SRGB: return nozzle::texture_format::bgra8_srgb;
        case DXGI_FORMAT_R16_UNORM: return nozzle::texture_format::r16_unorm;
        case DXGI_FORMAT_R16G16_UNORM: return nozzle::texture_format::rg16_unorm;
        case DXGI_FORMAT_R16G16B16A16_UNORM: return nozzle::texture_format::rgba16_unorm;
        case DXGI_FORMAT_R16_FLOAT: return nozzle::texture_format::r16_float;
        case DXGI_FORMAT_R16G16_FLOAT: return nozzle::texture_format::rg16_float;
        case DXGI_FORMAT_R16G16B16A16_FLOAT: return nozzle::texture_format::rgba16_float;
        case DXGI_FORMAT_R32_FLOAT: return nozzle::texture_format::r32_float;
        case DXGI_FORMAT_R32G32_FLOAT: return nozzle::texture_format::rg32_float;
        case DXGI_FORMAT_R32G32B32A32_FLOAT: return nozzle::texture_format::rgba32_float;
        case DXGI_FORMAT_R32_UINT: return nozzle::texture_format::r32_uint;
        case DXGI_FORMAT_R32G32B32A32_UINT: return nozzle::texture_format::rgba32_uint;
        case DXGI_FORMAT_D32_FLOAT: return nozzle::texture_format::depth32_float;
        default: return nozzle::texture_format::unknown;
    }
}

std::string spout_status_message() {
    std::ostringstream out;
    out << "SpoutDX compile/link path is enabled from Spout2 " << spout_upstream_tag
        << " (" << spout_upstream_sha << ", " << spout_license << ") at "
        << spout_sdk_path << "; live host smoke evidence is still required";
    return out.str();
}

std::vector<external_source_info> enumerate_spout_sources() {
    spoutDX spout{};
    std::vector<std::string> sender_names = spout.GetSenderList();
    std::vector<external_source_info> sources{};
    sources.reserve(sender_names.size());
    for (std::size_t index = 0; index < sender_names.size(); ++index) {
        external_source_info source{};
        source.backend = external_source_backend::spout;
        source.display_name = sender_names[index];
        source.server_name = sender_names[index];
        source.platform_index = index;
        sources.push_back(std::move(source));
    }
    return sources;
}

bridge_run_result run_spout_to_nozzle(const app_config &config) {
    bridge_run_result result{};
    const std::vector<external_source_info> sources = enumerate_spout_sources();
    const source_selection_result selection = select_external_source(
        external_source_backend::spout,
        config.source_name,
        sources);
    if (selection.status != source_selection_status::selected) {
        result.error_message = format_source_selection_error(selection, sources);
        result.exit_code = 8;
        return result;
    }
    const std::string selected_sender_name = sources[selection.selected_index].server_name;

    spoutDX receiver{};
    receiver.SetReceiverName(selected_sender_name.c_str());
    if (!receiver.OpenDirectX11() || !receiver.GetDX11Device()) {
        result.error_message = "failed to initialize SpoutDX D3D11 receiver device";
        result.exit_code = 11;
        return result;
    }

    nozzle::sender sender{};
    bool sender_created = false;
    auto deadline = make_deadline(config);

    while (config.frame_limit == 0 || result.frames_processed < config.frame_limit) {
        if (!receiver.ReceiveTexture()) {
            if (deadline_expired(deadline)) {
                result.error_message = "Spout sender not found or produced no D3D11 texture before timeout: " + selected_sender_name;
                result.exit_code = 8;
                return result;
            }
            std::this_thread::sleep_for(idle_sleep(config));
            continue;
        }

        if (!receiver.IsFrameNew()) {
            if (deadline_expired(deadline)) {
                result.error_message = "Spout sender connected but produced no new frame before timeout: " + selected_sender_name;
                result.exit_code = 8;
                return result;
            }
            std::this_thread::sleep_for(idle_sleep(config));
            continue;
        }

        ID3D11Texture2D *texture = receiver.GetSenderTexture();
        if (!texture) {
            if (deadline_expired(deadline)) {
                result.error_message = "Spout receiver connected but returned no D3D11 texture before timeout";
                result.exit_code = 12;
                return result;
            }
            std::this_thread::sleep_for(idle_sleep(config));
            continue;
        }

        D3D11_TEXTURE2D_DESC texture_desc{};
        texture->GetDesc(&texture_desc);
        const nozzle::texture_format format = format_from_dxgi(texture_desc.Format);
        if (format == nozzle::texture_format::unknown) {
            result.error_message = "unsupported Spout DXGI texture format: " + std::to_string(static_cast<unsigned int>(texture_desc.Format));
            result.exit_code = 4;
            return result;
        }

        if (!sender_created) {
            nozzle::sender_desc sender_desc{};
            sender_desc.name = config.target_sender_name;
            sender_desc.application_name = "nozzle-spout";
            sender_desc.native_device.backend = nozzle::backend_type::d3d11;
            sender_desc.native_device.device = receiver.GetDX11Device();
            sender_desc.native_device.context = receiver.GetDX11Context();
            auto sender_result = nozzle::sender::create(sender_desc);
            if (!sender_result.ok()) {
                result.error_message = "failed to create nozzle sender for Spout receiver: " + sender_result.error().message;
                result.exit_code = 11;
                return result;
            }
            sender = std::move(sender_result.value());
            sender_created = true;
        }

        auto publish_result = sender.publish_native_texture(
            texture,
            texture_desc.Width,
            texture_desc.Height,
            format);
        if (!publish_result.ok()) {
            result.error_message = "failed to publish Spout D3D11 texture to nozzle: " + publish_result.error().message;
            result.exit_code = 12;
            return result;
        }

        deadline = make_deadline(config);
        result.frames_processed = result.frames_processed + 1;
    }

    result.ok = true;
    result.exit_code = 0;
    result.status_message = "Spout -> nozzle bridge completed";
    return result;
}

bridge_run_result run_nozzle_to_spout(const app_config &config) {
    bridge_run_result result{};
    d3d11_device_reference spout_device_reference{};
    spoutDX sender{};
    sender.SetSenderName(config.target_sender_name.c_str());
    bool spout_opened = false;
    nozzle::receiver receiver{};
    bool receiver_created = false;
    auto deadline = make_deadline(config);

    while (!receiver_created) {
        nozzle::receiver_desc receiver_desc{};
        receiver_desc.name = config.source_name;
        receiver_desc.application_name = "nozzle-spout";
        auto receiver_result = nozzle::receiver::create(receiver_desc);
        if (receiver_result.ok()) {
            receiver = std::move(receiver_result.value());
            receiver_created = true;
            break;
        }
        if (deadline_expired(deadline)) {
            result.error_message = "nozzle source not found before timeout: " + config.source_name;
            result.exit_code = 8;
            return result;
        }
        std::this_thread::sleep_for(idle_sleep(config));
    }

    auto frame_deadline = make_deadline(config);
    while (config.frame_limit == 0 || result.frames_processed < config.frame_limit) {
        nozzle::acquire_desc acquire_desc{};
        acquire_desc.timeout_ms = config.timeout_ms;
        auto frame_result = receiver.acquire_frame(acquire_desc);
        if (!frame_result.ok()) {
            if (frame_result.error().code == nozzle::ErrorCode::Timeout ||
                frame_result.error().code == nozzle::ErrorCode::SenderNotFound) {
                if (deadline_expired(frame_deadline)) {
                    result.error_message = "nozzle source produced no D3D11 frame before timeout: " + config.source_name;
                    result.exit_code = 8;
                    return result;
                }
                std::this_thread::sleep_for(idle_sleep(config));
                continue;
            }
            result.error_message = "failed to acquire nozzle D3D11 frame: " + frame_result.error().message;
            result.exit_code = 12;
            return result;
        }

        nozzle::frame frame = std::move(frame_result.value());
        ID3D11Texture2D *texture = nozzle::d3d11::get_texture(frame.get_texture());
        if (!texture) {
            result.error_message = "nozzle frame has no D3D11 texture";
            result.exit_code = 11;
            return result;
        }

        if (!spout_opened) {
            ID3D11Device *texture_device = nullptr;
            texture->GetDevice(&texture_device);
            if (!texture_device) {
                result.error_message = "failed to get D3D11 device from nozzle frame texture";
                result.exit_code = 11;
                return result;
            }
            spout_device_reference.reset(texture_device);
            if (!sender.OpenDirectX11(spout_device_reference.device) || !sender.GetDX11Device()) {
                result.error_message = "failed to initialize SpoutDX sender on nozzle frame D3D11 device";
                result.exit_code = 11;
                return result;
            }
            spout_opened = true;
        }

        if (!sender.SendTexture(texture)) {
            result.error_message = "SpoutDX SendTexture failed";
            result.exit_code = 12;
            return result;
        }
        frame_deadline = make_deadline(config);
        result.frames_processed = result.frames_processed + 1;
    }

    result.ok = true;
    result.exit_code = 0;
    result.status_message = "nozzle -> Spout bridge completed";
    return result;
}

} // namespace

bridge_status query_platform_bridge_status(const app_config &config) {
    bridge_status status{};
    status.platform_name = "Windows";
    status.external_system_name = "Spout";
    status.bridge_available = true;
    status.runtime_supported = false;
    status.width = config.requested_width;
    status.height = config.requested_height;
    status.frames_per_second = 0.0;
    status.status_message = spout_status_message();
    status.error_message = "SpoutDX D3D11 bridge is compiled and linked, but live host smoke evidence is not attached yet";
    return status;
}

std::vector<std::string> list_platform_sources() {
    const std::vector<external_source_info> spout_sources = enumerate_spout_sources();
    std::vector<std::string> sources{};
    sources.reserve(spout_sources.size());
    for (const external_source_info &source : spout_sources) {
        sources.push_back(format_external_source_list_line(source, spout_sources));
    }
    return sources;
}

bridge_run_result run_platform_bridge(const app_config &config) {
    if (config.direction == bridge_direction::external_to_nozzle) {
        return run_spout_to_nozzle(config);
    }
    return run_nozzle_to_spout(config);
}

} // namespace nozzle_spout_syphon
