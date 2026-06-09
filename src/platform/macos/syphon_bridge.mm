#include <app/status_report.hpp>
#include <app/source_selection.hpp>

#include <nozzle/backends/metal.hpp>
#include <nozzle/receiver.hpp>
#include <nozzle/sender.hpp>

#import <Foundation/Foundation.h>
#import <Metal/Metal.h>
#import <Syphon/Syphon.h>

#include <chrono>
#include <sstream>
#include <thread>
#include <vector>

namespace nozzle_spout_syphon {

namespace {

NSString *to_ns_string(const std::string &value) {
    return [NSString stringWithUTF8String:value.c_str()];
}

std::string to_std_string(NSString *value) {
    if (!value) {
        return {};
    }
    const char *utf8 = [value UTF8String];
    return utf8 ? std::string{utf8} : std::string{};
}

nozzle::texture_format format_from_mtl_texture(id<MTLTexture> texture) {
    if (!texture) {
        return nozzle::texture_format::unknown;
    }
    switch ([texture pixelFormat]) {
        case MTLPixelFormatRGBA8Unorm: return nozzle::texture_format::rgba8_unorm;
        case MTLPixelFormatBGRA8Unorm: return nozzle::texture_format::bgra8_unorm;
        case MTLPixelFormatRGBA8Unorm_sRGB: return nozzle::texture_format::rgba8_srgb;
        case MTLPixelFormatBGRA8Unorm_sRGB: return nozzle::texture_format::bgra8_srgb;
        case MTLPixelFormatR8Unorm: return nozzle::texture_format::r8_unorm;
        case MTLPixelFormatRG8Unorm: return nozzle::texture_format::rg8_unorm;
        case MTLPixelFormatR16Unorm: return nozzle::texture_format::r16_unorm;
        case MTLPixelFormatRG16Unorm: return nozzle::texture_format::rg16_unorm;
        case MTLPixelFormatRGBA16Unorm: return nozzle::texture_format::rgba16_unorm;
        case MTLPixelFormatR16Float: return nozzle::texture_format::r16_float;
        case MTLPixelFormatRG16Float: return nozzle::texture_format::rg16_float;
        case MTLPixelFormatRGBA16Float: return nozzle::texture_format::rgba16_float;
        case MTLPixelFormatR32Float: return nozzle::texture_format::r32_float;
        case MTLPixelFormatRG32Float: return nozzle::texture_format::rg32_float;
        case MTLPixelFormatRGBA32Float: return nozzle::texture_format::rgba32_float;
        case MTLPixelFormatR32Uint: return nozzle::texture_format::r32_uint;
        case MTLPixelFormatRGBA32Uint: return nozzle::texture_format::rgba32_uint;
        case MTLPixelFormatDepth32Float: return nozzle::texture_format::depth32_float;
        default: return nozzle::texture_format::unknown;
    }
}

std::vector<external_source_info> enumerate_syphon_sources() {
    std::vector<external_source_info> sources{};
    NSArray<NSDictionary<NSString *, id<NSCoding>> *> *servers =
        [[SyphonServerDirectory sharedDirectory] servers];
    sources.reserve(static_cast<std::size_t>([servers count]));
    for (NSUInteger index = 0; index < [servers count]; ++index) {
        NSDictionary<NSString *, id<NSCoding>> *server = [servers objectAtIndex:index];
        NSString *server_name = (NSString *)[server objectForKey:SyphonServerDescriptionNameKey];
        NSString *app_name = (NSString *)[server objectForKey:SyphonServerDescriptionAppNameKey];
        external_source_info source{};
        source.backend = external_source_backend::syphon;
        source.server_name = to_std_string(server_name);
        source.app_name = to_std_string(app_name);
        source.display_name = source.server_name.empty() ? source.app_name : source.server_name;
        source.platform_index = static_cast<std::size_t>(index);
        sources.push_back(std::move(source));
    }
    return sources;
}

std::chrono::steady_clock::time_point make_source_selection_deadline(const app_config &config) {
    const std::uint64_t timeout_ms = config.timeout_ms == 0 ? 1000 : config.timeout_ms;
    return std::chrono::steady_clock::now() + std::chrono::milliseconds(timeout_ms);
}

bool source_selection_deadline_expired(std::chrono::steady_clock::time_point deadline) {
    return deadline <= std::chrono::steady_clock::now();
}

std::chrono::milliseconds source_selection_idle_sleep(const app_config &config) {
    return std::chrono::milliseconds(config.idle_sleep_ms == 0 ? 1 : config.idle_sleep_ms);
}

bridge_run_result run_syphon_to_nozzle(const app_config &config, id<MTLDevice> device) {
    bridge_run_result result{};
    std::vector<external_source_info> sources{};
    source_selection_result selection{};
    const auto selection_deadline = make_source_selection_deadline(config);
    for (;;) {
        sources = enumerate_syphon_sources();
        selection = select_external_source(external_source_backend::syphon, config.source_name, sources);
        if (selection.status == source_selection_status::selected) {
            break;
        }
        if (selection.status == source_selection_status::ambiguous ||
            selection.status == source_selection_status::unsupported_selector ||
            source_selection_deadline_expired(selection_deadline)) {
            result.error_message = format_source_selection_error(selection, sources);
            result.exit_code = 8;
            return result;
        }
        std::this_thread::sleep_for(source_selection_idle_sleep(config));
    }

    NSArray<NSDictionary<NSString *, id<NSCoding>> *> *servers =
        [[SyphonServerDirectory sharedDirectory] servers];
    const external_source_info &selected_source = sources[selection.selected_index];
    if (selected_source.platform_index >= static_cast<std::size_t>([servers count])) {
        result.error_message = "selected Syphon source disappeared before client creation: " + format_source_candidate(selected_source);
        result.exit_code = 8;
        return result;
    }
    NSDictionary<NSString *, id<NSCoding>> *server_description =
        [servers objectAtIndex:static_cast<NSUInteger>(selected_source.platform_index)];

    SyphonMetalClient *client = [[SyphonMetalClient alloc]
        initWithServerDescription:server_description
                           device:device
                          options:nil
                  newFrameHandler:nil];
    if (!client || ![client isValid]) {
        if (client) {
            [client release];
        }
        result.error_message = "failed to create SyphonMetalClient";
        result.exit_code = 11;
        return result;
    }

    nozzle::sender_desc sender_desc{};
    sender_desc.name = config.target_sender_name;
    sender_desc.application_name = "nozzle-syphon";
    sender_desc.native_device.backend = nozzle::backend_type::metal;
    sender_desc.native_device.device = (__bridge void *)device;
    auto sender_result = nozzle::sender::create(sender_desc);
    if (!sender_result.ok()) {
        [client stop];
        [client release];
        result.error_message = "failed to create nozzle sender: " + sender_result.error().message;
        result.exit_code = 11;
        return result;
    }
    nozzle::sender sender = std::move(sender_result.value());

    while (config.frame_limit == 0 || result.frames_processed < config.frame_limit) {
        @autoreleasepool {
            if (![client hasNewFrame]) {
                std::this_thread::sleep_for(std::chrono::milliseconds(config.idle_sleep_ms));
                continue;
            }

            id<MTLTexture> texture = [client newFrameImage];
            if (!texture) {
                std::this_thread::sleep_for(std::chrono::milliseconds(config.idle_sleep_ms));
                continue;
            }

            nozzle::texture_format format = format_from_mtl_texture(texture);
            if (format == nozzle::texture_format::unknown) {
                [texture release];
                result.error_message = "unsupported Syphon Metal pixel format";
                result.exit_code = 4;
                [client stop];
                [client release];
                return result;
            }

            auto publish_result = sender.publish_native_texture(
                (__bridge void *)texture,
                static_cast<uint32_t>([texture width]),
                static_cast<uint32_t>([texture height]),
                format);
            [texture release];
            if (!publish_result.ok()) {
                result.error_message = "failed to publish Syphon frame to nozzle: " + publish_result.error().message;
                result.exit_code = 12;
                [client stop];
                [client release];
                return result;
            }
            result.frames_processed = result.frames_processed + 1;
        }
    }

    [client stop];
    [client release];
    result.ok = true;
    result.exit_code = 0;
    result.status_message = "Syphon -> nozzle bridge completed";
    return result;
}

bridge_run_result run_nozzle_to_syphon(const app_config &config, id<MTLDevice> device) {
    bridge_run_result result{};
    SyphonMetalServer *server = [[SyphonMetalServer alloc]
        initWithName:to_ns_string(config.target_sender_name)
              device:device
             options:nil];
    if (!server) {
        result.error_message = "failed to create SyphonMetalServer";
        result.exit_code = 11;
        return result;
    }

    id<MTLCommandQueue> command_queue = [device newCommandQueue];
    if (!command_queue) {
        [server stop];
        [server release];
        result.error_message = "failed to create Metal command queue";
        result.exit_code = 11;
        return result;
    }

    nozzle::receiver_desc receiver_desc{};
    receiver_desc.name = config.source_name;
    receiver_desc.application_name = "nozzle-syphon";
    auto receiver_result = nozzle::receiver::create(receiver_desc);
    if (!receiver_result.ok()) {
        [command_queue release];
        [server stop];
        [server release];
        result.error_message = "failed to create nozzle receiver: " + receiver_result.error().message;
        result.exit_code = 11;
        return result;
    }
    nozzle::receiver receiver = std::move(receiver_result.value());

    while (config.frame_limit == 0 || result.frames_processed < config.frame_limit) {
        @autoreleasepool {
            nozzle::acquire_desc acquire_desc{};
            acquire_desc.timeout_ms = config.timeout_ms;
            auto frame_result = receiver.acquire_frame(acquire_desc);
            if (!frame_result.ok()) {
                if (frame_result.error().code == nozzle::ErrorCode::Timeout ||
                    frame_result.error().code == nozzle::ErrorCode::SenderNotFound) {
                    std::this_thread::sleep_for(std::chrono::milliseconds(config.idle_sleep_ms));
                    continue;
                }
                result.error_message = "failed to acquire nozzle frame: " + frame_result.error().message;
                result.exit_code = 12;
                [command_queue release];
                [server stop];
                [server release];
                return result;
            }

            nozzle::frame frame = std::move(frame_result.value());
            const nozzle::frame_info info = frame.info();
            id<MTLTexture> texture = (__bridge id<MTLTexture>)nozzle::metal::get_texture(frame.get_texture());
            if (!texture) {
                result.error_message = "nozzle frame has no Metal texture";
                result.exit_code = 11;
                [command_queue release];
                [server stop];
                [server release];
                return result;
            }

            id<MTLCommandBuffer> command_buffer = [command_queue commandBuffer];
            if (!command_buffer) {
                result.error_message = "failed to create Metal command buffer";
                result.exit_code = 11;
                [command_queue release];
                [server stop];
                [server release];
                return result;
            }

            [server publishFrameTexture:texture
                        onCommandBuffer:command_buffer
                            imageRegion:NSMakeRect(0.0, 0.0, info.width, info.height)
                                flipped:NO];
            [command_buffer commit];
            [command_buffer waitUntilCompleted];
            if ([command_buffer status] != MTLCommandBufferStatusCompleted) {
                NSError *error = [command_buffer error];
                result.error_message = "Metal command buffer failed while publishing nozzle frame to Syphon";
                if (error) {
                    result.error_message += ": " + to_std_string([error localizedDescription]);
                }
                result.exit_code = 12;
                [command_queue release];
                [server stop];
                [server release];
                return result;
            }
            result.frames_processed = result.frames_processed + 1;
        }
    }

    [command_queue release];
    [server stop];
    [server release];
    result.ok = true;
    result.exit_code = 0;
    result.status_message = "nozzle -> Syphon bridge completed";
    return result;
}

} // namespace

bridge_status query_platform_bridge_status(const app_config &config) {
    id<MTLDevice> device = MTLCreateSystemDefaultDevice();
    const bool has_default_metal_device = device != nil;
    if (device) {
        [device release];
    }

    bridge_status status{};
    status.platform_name = "macOS";
    status.external_system_name = "Syphon";
    status.bridge_available = true;
    status.runtime_supported = false;
    status.width = config.requested_width;
    status.height = config.requested_height;
    status.frames_per_second = 0.0;
    status.status_message = has_default_metal_device
        ? "Syphon Metal runtime bridge is compiled; runtime support remains unclaimed until host smoke evidence is attached"
        : "Syphon Metal runtime bridge is compiled, but this host has no default Metal device";
    if (!has_default_metal_device) {
        status.error_message = "MTLCreateSystemDefaultDevice returned nil; --run cannot work on this host";
    }
    return status;
}

std::vector<std::string> list_platform_sources() {
    std::vector<std::string> sources{};
    @autoreleasepool {
        const std::vector<external_source_info> syphon_sources = enumerate_syphon_sources();
        for (const external_source_info &source : syphon_sources) {
            sources.push_back(format_external_source_list_line(source, syphon_sources));
        }
    }
    return sources;
}

bridge_run_result run_platform_bridge(const app_config &config) {
    @autoreleasepool {
        id<MTLDevice> device = MTLCreateSystemDefaultDevice();
        if (!device) {
            bridge_run_result result{};
            result.error_message = "failed to create default Metal device";
            result.exit_code = 11;
            return result;
        }

        bridge_run_result result{};
        if (config.direction == bridge_direction::external_to_nozzle) {
            result = run_syphon_to_nozzle(config, device);
        } else {
            result = run_nozzle_to_syphon(config, device);
        }
        [device release];
        return result;
    }
}

} // namespace nozzle_spout_syphon
