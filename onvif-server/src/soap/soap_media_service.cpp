#include "soap_common.h"

#include "media_model.h"
#include "rail_client.h"
#include "utils.h"

#include <algorithm>
#include <array>
#include <cstddef>
#include <iostream>
#include <optional>
#include <string>

namespace afterveda_onvif {
namespace {

struct SupportedResolution {
    int width;
    int height;
};

constexpr std::array<SupportedResolution, 3> kSupportedResolutions{{
    {640, 360},
    {1280, 720},
    {1920, 1080},
}};
constexpr int kSupportedFrameRate = 30;
constexpr int kMinBitrateKbps = 800;
constexpr int kMaxBitrateKbps = 5000;
constexpr float kVideoQuality = 4.0F;

std::string string_value(const char* value) {
    return value == nullptr ? std::string{} : std::string{value};
}

bool media1_stream_setup_supported(const _trt__GetStreamUri* request) {
    if (request == nullptr || request->StreamSetup == nullptr ||
        request->StreamSetup->Transport == nullptr) {
        return false;
    }
    const std::string stream = string_value(request->StreamSetup->Stream);
    const std::string protocol = string_value(request->StreamSetup->Transport->Protocol);
    return stream == "RTP-Unicast" && (protocol == "RTSP" || protocol == "TCP");
}

bool media2_protocol_supported(const char* protocol) {
    return string_value(protocol) == "RTSP";
}

bool supported_resolution(int width, int height) {
    for (const auto& resolution : kSupportedResolutions) {
        if (resolution.width == width && resolution.height == height) {
            return true;
        }
    }
    return false;
}

std::optional<VideoProfile> video_config_from_encoder_request(
    const MediaModel& model,
    const _trt__SetVideoEncoderConfiguration* request,
    const std::string& token) {
    if (request == nullptr || request->Configuration == nullptr) {
        return std::nullopt;
    }

    const tt__VideoEncoderConfiguration& config = *request->Configuration;
    VideoProfile candidate = profile_for_token(model, token);

    if (config.Encoding != nullptr && string_value(config.Encoding) != "H264") {
        return std::nullopt;
    }

    if (config.Resolution != nullptr) {
        candidate.width = config.Resolution->Width;
        candidate.height = config.Resolution->Height;
    }
    if (config.RateControl != nullptr) {
        candidate.fps = config.RateControl->FrameRateLimit;
        candidate.bitrate_kbps = std::clamp(
            config.RateControl->BitrateLimit,
            kMinBitrateKbps,
            kMaxBitrateKbps);
    }

    if (token.empty() &&
        config.Resolution == nullptr &&
        config.RateControl == nullptr) {
        return std::nullopt;
    }

    if (!supported_resolution(candidate.width, candidate.height) ||
        candidate.fps != kSupportedFrameRate) {
        return std::nullopt;
    }
    candidate.token = "main";
    candidate.name = "main";
    return candidate;
}

std::string encoder_request_profile_token(const _trt__SetVideoEncoderConfiguration* request) {
    if (request == nullptr || request->Configuration == nullptr || request->Configuration->token == nullptr) {
        return {};
    }
    return profile_token_from_encoder_token(request->Configuration->token);
}

template <typename T>
T* soap_make_array(soap* ctx, std::size_t count) {
    if (count == 0) {
        return nullptr;
    }
    void* memory = soap_malloc(ctx, sizeof(T) * count);
    if (memory == nullptr) {
        return nullptr;
    }
    T* values = static_cast<T*>(memory);
    for (std::size_t i = 0; i < count; ++i) {
        new (&values[i]) T();
    }
    return values;
}

tt__VideoResolution* make_video_resolution(soap* ctx, int width, int height) {
    tt__VideoResolution* resolution = soap_make<tt__VideoResolution>(ctx);
    if (resolution == nullptr) {
        return nullptr;
    }
    resolution->Width = width;
    resolution->Height = height;
    return resolution;
}

tt__IntRange* make_int_range(soap* ctx, int min, int max) {
    tt__IntRange* range = soap_make<tt__IntRange>(ctx);
    if (range == nullptr) {
        return nullptr;
    }
    range->Min = min;
    range->Max = max;
    return range;
}

tt__FloatRange* make_float_range(soap* ctx, float min, float max) {
    tt__FloatRange* range = soap_make<tt__FloatRange>(ctx);
    if (range == nullptr) {
        return nullptr;
    }
    range->Min = min;
    range->Max = max;
    return range;
}

tt__IntRectangle* make_bounds(soap* ctx, const VideoProfile& profile) {
    tt__IntRectangle* bounds = soap_make<tt__IntRectangle>(ctx);
    if (bounds == nullptr) {
        return nullptr;
    }
    bounds->x = 0;
    bounds->y = 0;
    bounds->width = profile.width;
    bounds->height = profile.height;
    return bounds;
}

tt__MulticastConfiguration* make_multicast(soap* ctx) {
    tt__MulticastConfiguration* multicast = soap_make<tt__MulticastConfiguration>(ctx);
    if (multicast == nullptr) {
        return nullptr;
    }
    multicast->Address = soap_make<tt__IPAddress>(ctx);
    if (multicast->Address == nullptr) {
        return nullptr;
    }
    multicast->Address->Type = soap_strdup(ctx, "IPv4");
    multicast->Address->IPv4Address = soap_strdup(ctx, "0.0.0.0");
    multicast->Port = 0;
    multicast->TTL = 1;
    multicast->AutoStart = false;
    return multicast;
}

tt__VideoSourceConfiguration* make_video_source_configuration(soap* ctx, const VideoProfile& profile) {
    tt__VideoSourceConfiguration* config = soap_make<tt__VideoSourceConfiguration>(ctx);
    if (config == nullptr) {
        return nullptr;
    }
    config->token = soap_strdup(ctx, kVideoSourceConfigToken);
    config->Name = soap_strdup(ctx, "PiCam");
    config->UseCount = 1;
    config->SourceToken = soap_strdup(ctx, kVideoSourceToken);
    config->Bounds = make_bounds(ctx, profile);
    if (config->token == nullptr || config->Name == nullptr || config->SourceToken == nullptr || config->Bounds == nullptr) {
        return nullptr;
    }
    return config;
}

tt__VideoEncoderConfiguration* make_video_encoder_configuration(soap* ctx, const VideoProfile& profile) {
    tt__VideoEncoderConfiguration* config = soap_make<tt__VideoEncoderConfiguration>(ctx);
    if (config == nullptr) {
        return nullptr;
    }
    const std::string token = encoder_token_for_profile(profile);
    const std::string name = profile.name + "-h264";
    config->token = soap_strdup(ctx, token.c_str());
    config->Name = soap_strdup(ctx, name.c_str());
    config->UseCount = 1;
    config->Encoding = soap_strdup(ctx, "H264");
    config->Resolution = make_video_resolution(ctx, profile.width, profile.height);
    config->Quality = kVideoQuality;
    config->RateControl = soap_make<tt__VideoRateControl>(ctx);
    config->H264 = soap_make<tt__H264Configuration>(ctx);
    config->Multicast = make_multicast(ctx);
    config->SessionTimeout = soap_strdup(ctx, kMediaSessionTimeout);
    if (config->token == nullptr || config->Name == nullptr || config->Encoding == nullptr ||
        config->Resolution == nullptr || config->RateControl == nullptr || config->H264 == nullptr ||
        config->Multicast == nullptr || config->SessionTimeout == nullptr) {
        return nullptr;
    }
    config->RateControl->FrameRateLimit = profile.fps;
    config->RateControl->EncodingInterval = 1;
    config->RateControl->BitrateLimit = profile.bitrate_kbps;
    config->H264->GovLength = profile.fps * 2;
    config->H264->H264Profile = soap_strdup(ctx, "Main");
    if (config->H264->H264Profile == nullptr) {
        return nullptr;
    }
    return config;
}

tt__PTZConfiguration* make_ptz_configuration(soap* ctx) {
    tt__PTZConfiguration* config = soap_make<tt__PTZConfiguration>(ctx);
    if (config == nullptr) {
        return nullptr;
    }
    config->token = soap_strdup(ctx, kPtzConfigToken);
    config->Name = soap_strdup(ctx, "PanTilt");
    config->UseCount = 1;
    config->NodeToken = soap_strdup(ctx, "ptz-node");
    config->DefaultContinuousPanTiltVelocitySpace = soap_strdup(ctx, "http://www.onvif.org/ver10/tptz/PanTiltSpaces/VelocityGenericSpace");
    config->DefaultRelativePanTiltTranslationSpace = soap_strdup(ctx, "http://www.onvif.org/ver10/tptz/PanTiltSpaces/TranslationGenericSpace");
    config->DefaultPTZTimeout = soap_strdup(ctx, "PT1S");
    if (config->token == nullptr || config->Name == nullptr || config->NodeToken == nullptr ||
        config->DefaultContinuousPanTiltVelocitySpace == nullptr ||
        config->DefaultRelativePanTiltTranslationSpace == nullptr ||
        config->DefaultPTZTimeout == nullptr) {
        return nullptr;
    }
    return config;
}

int fill_media1_profile(soap* ctx, tt__Profile& response, const VideoProfile& profile) {
    response.token = soap_strdup(ctx, profile.token.c_str());
    response.fixed = false;
    response.Name = soap_strdup(ctx, profile.name.c_str());
    response.VideoSourceConfiguration = make_video_source_configuration(ctx, profile);
    response.VideoEncoderConfiguration = make_video_encoder_configuration(ctx, profile);
    response.PTZConfiguration = make_ptz_configuration(ctx);
    if (response.token == nullptr || response.Name == nullptr ||
        response.VideoSourceConfiguration == nullptr ||
        response.VideoEncoderConfiguration == nullptr ||
        response.PTZConfiguration == nullptr) {
        return SOAP_EOM;
    }
    return SOAP_OK;
}

int fill_media1_profiles_response(soap* ctx, _trt__GetProfilesResponse& response, const MediaModel& model) {
    response.__sizeProfiles = static_cast<int>(model.profiles.size());
    response.Profiles = soap_make_array<tt__Profile>(ctx, model.profiles.size());
    if (!model.profiles.empty() && response.Profiles == nullptr) {
        return SOAP_EOM;
    }
    for (std::size_t i = 0; i < model.profiles.size(); ++i) {
        if (const int err = fill_media1_profile(ctx, response.Profiles[i], model.profiles[i]); err != SOAP_OK) {
            return err;
        }
    }
    return SOAP_OK;
}

int fill_media1_get_profile_response(soap* ctx, _trt__GetProfileResponse& response, const VideoProfile& profile) {
    response.Profile = soap_make<tt__Profile>(ctx);
    if (response.Profile == nullptr) {
        return SOAP_EOM;
    }
    return fill_media1_profile(ctx, *response.Profile, profile);
}

int fill_media1_stream_uri_response(soap* ctx, _trt__GetStreamUriResponse& response, const MediaModel& model) {
    response.MediaUri = soap_make<tt__MediaUri>(ctx);
    if (response.MediaUri == nullptr) {
        return SOAP_EOM;
    }
    response.MediaUri->Uri = soap_strdup(ctx, model.rtsp_uri.c_str());
    response.MediaUri->InvalidAfterConnect = false;
    response.MediaUri->InvalidAfterReboot = false;
    response.MediaUri->Timeout = soap_strdup(ctx, kMediaSessionTimeout);
    if (response.MediaUri->Uri == nullptr || response.MediaUri->Timeout == nullptr) {
        return SOAP_EOM;
    }
    return SOAP_OK;
}

int fill_media1_video_sources_response(soap* ctx, _trt__GetVideoSourcesResponse& response, const MediaModel& model) {
    const VideoProfile profile = profile_for_token(model, "main");
    response.__sizeVideoSources = 1;
    response.VideoSources = soap_make_array<tt__VideoSource>(ctx, 1);
    if (response.VideoSources == nullptr) {
        return SOAP_EOM;
    }
    response.VideoSources[0].token = soap_strdup(ctx, kVideoSourceToken);
    response.VideoSources[0].Framerate = static_cast<float>(profile.fps);
    response.VideoSources[0].Resolution = make_video_resolution(ctx, profile.width, profile.height);
    if (response.VideoSources[0].token == nullptr || response.VideoSources[0].Resolution == nullptr) {
        return SOAP_EOM;
    }
    return SOAP_OK;
}

int fill_media1_video_encoder_configuration_response(
    soap* ctx,
    _trt__GetVideoEncoderConfigurationResponse& response,
    const VideoProfile& profile) {
    response.Configuration = make_video_encoder_configuration(ctx, profile);
    return response.Configuration == nullptr ? SOAP_EOM : SOAP_OK;
}

int fill_media1_video_encoder_configurations_response(
    soap* ctx,
    _trt__GetVideoEncoderConfigurationsResponse& response,
    const MediaModel& model) {
    response.__sizeConfigurations = static_cast<int>(model.profiles.size());
    response.Configurations = soap_make_array<tt__VideoEncoderConfiguration>(ctx, model.profiles.size());
    if (!model.profiles.empty() && response.Configurations == nullptr) {
        return SOAP_EOM;
    }
    for (std::size_t i = 0; i < model.profiles.size(); ++i) {
        tt__VideoEncoderConfiguration* config = make_video_encoder_configuration(ctx, model.profiles[i]);
        if (config == nullptr) {
            return SOAP_EOM;
        }
        response.Configurations[i] = *config;
    }
    return SOAP_OK;
}

int fill_media1_service_capabilities_response(soap* ctx, _trt__GetServiceCapabilitiesResponse& response) {
    response.Capabilities = soap_make<trt__Capabilities>(ctx);
    if (response.Capabilities == nullptr) {
        return SOAP_EOM;
    }
    response.Capabilities->SnapshotUri = false;
    response.Capabilities->Rotation = false;
    response.Capabilities->VideoSourceMode = false;
    response.Capabilities->OSD = true;
    response.Capabilities->TemporaryOSDText = false;
    response.Capabilities->EXICompression = false;
    return SOAP_OK;
}

int fill_media1_video_source_configurations_response(
    soap* ctx,
    _trt__GetVideoSourceConfigurationsResponse& response,
    const MediaModel& model) {
    response.__sizeConfigurations = 1;
    response.Configurations = soap_make_array<tt__VideoSourceConfiguration>(ctx, 1);
    if (response.Configurations == nullptr) {
        return SOAP_EOM;
    }
    tt__VideoSourceConfiguration* config = make_video_source_configuration(ctx, profile_for_token(model, "main"));
    if (config == nullptr) {
        return SOAP_EOM;
    }
    response.Configurations[0] = *config;
    return SOAP_OK;
}

int fill_media1_video_source_configuration_response(
    soap* ctx,
    _trt__GetVideoSourceConfigurationResponse& response,
    const MediaModel& model) {
    response.Configuration = make_video_source_configuration(ctx, profile_for_token(model, "main"));
    return response.Configuration == nullptr ? SOAP_EOM : SOAP_OK;
}

int fill_media1_video_source_configuration_options_response(
    soap* ctx,
    _trt__GetVideoSourceConfigurationOptionsResponse& response,
    const MediaModel& model) {
    const VideoProfile profile = profile_for_token(model, "main");
    response.Options = soap_make<tt__VideoSourceConfigurationOptions>(ctx);
    if (response.Options == nullptr) {
        return SOAP_EOM;
    }

    response.Options->BoundsRange = soap_make<tt__IntRectangleRange>(ctx);
    response.Options->__sizeVideoSourceTokensAvailable = 1;
    response.Options->VideoSourceTokensAvailable = soap_make_array<char*>(ctx, 1);
    if (response.Options->BoundsRange == nullptr || response.Options->VideoSourceTokensAvailable == nullptr) {
        return SOAP_EOM;
    }

    response.Options->BoundsRange->XRange = make_int_range(ctx, 0, 0);
    response.Options->BoundsRange->YRange = make_int_range(ctx, 0, 0);
    response.Options->BoundsRange->WidthRange = make_int_range(ctx, profile.width, profile.width);
    response.Options->BoundsRange->HeightRange = make_int_range(ctx, profile.height, profile.height);
    response.Options->VideoSourceTokensAvailable[0] = soap_strdup(ctx, kVideoSourceToken);
    if (response.Options->BoundsRange->XRange == nullptr ||
        response.Options->BoundsRange->YRange == nullptr ||
        response.Options->BoundsRange->WidthRange == nullptr ||
        response.Options->BoundsRange->HeightRange == nullptr ||
        response.Options->VideoSourceTokensAvailable[0] == nullptr) {
        return SOAP_EOM;
    }
    return SOAP_OK;
}

tt__H264Options* make_h264_options(soap* ctx) {
    tt__H264Options* h264 = soap_make<tt__H264Options>(ctx);
    if (h264 == nullptr) {
        return nullptr;
    }

    h264->__sizeResolutionsAvailable = static_cast<int>(kSupportedResolutions.size());
    h264->ResolutionsAvailable = soap_make_array<tt__VideoResolution>(ctx, kSupportedResolutions.size());
    h264->__sizeH264ProfilesSupported = 1;
    h264->H264ProfilesSupported = soap_make_array<char*>(ctx, 1);
    if (h264->ResolutionsAvailable == nullptr || h264->H264ProfilesSupported == nullptr) {
        return nullptr;
    }

    for (std::size_t i = 0; i < kSupportedResolutions.size(); ++i) {
        h264->ResolutionsAvailable[i].Width = kSupportedResolutions[i].width;
        h264->ResolutionsAvailable[i].Height = kSupportedResolutions[i].height;
    }
    h264->GovLengthRange = make_int_range(ctx, kSupportedFrameRate * 2, kSupportedFrameRate * 2);
    h264->FrameRateRange = make_int_range(ctx, kSupportedFrameRate, kSupportedFrameRate);
    h264->EncodingIntervalRange = make_int_range(ctx, 1, 1);
    h264->H264ProfilesSupported[0] = soap_strdup(ctx, "Main");
    if (h264->GovLengthRange == nullptr ||
        h264->FrameRateRange == nullptr ||
        h264->EncodingIntervalRange == nullptr ||
        h264->H264ProfilesSupported[0] == nullptr) {
        return nullptr;
    }
    return h264;
}

int fill_media1_video_encoder_configuration_options_response(
    soap* ctx,
    _trt__GetVideoEncoderConfigurationOptionsResponse& response,
    const MediaModel&) {
    response.Options = soap_make<tt__VideoEncoderConfigurationOptions>(ctx);
    if (response.Options == nullptr) {
        return SOAP_EOM;
    }

    response.Options->QualityRange = make_float_range(ctx, kVideoQuality, kVideoQuality);
    response.Options->H264 = make_h264_options(ctx);
    response.Options->Extension = nullptr;
    response.Options->BitrateRange = make_int_range(ctx, kMinBitrateKbps, kMaxBitrateKbps);
    if (response.Options->QualityRange == nullptr ||
        response.Options->H264 == nullptr ||
        response.Options->BitrateRange == nullptr) {
        return SOAP_EOM;
    }
    return SOAP_OK;
}

bool media2_type_requested(const _tr2__GetProfiles* request, const char* type) {
    if (request == nullptr || request->__sizeType == 0 || request->Type == nullptr) {
        return false;
    }
    for (int i = 0; i < request->__sizeType; ++i) {
        const std::string requested = string_value(request->Type[i]);
        if (requested == "All" || requested == type) {
            return true;
        }
    }
    return false;
}

int fill_media2_profile(soap* ctx, tr2__MediaProfile& response, const VideoProfile& profile,
                        const _tr2__GetProfiles* request) {
    response.token = soap_strdup(ctx, profile.token.c_str());
    response.fixed = false;
    response.Name = soap_strdup(ctx, profile.name.c_str());
    const bool include_video_source = media2_type_requested(request, "VideoSource");
    const bool include_video_encoder = media2_type_requested(request, "VideoEncoder");
    response.Configurations = (include_video_source || include_video_encoder)
        ? soap_make<tr2__ConfigurationSet>(ctx)
        : nullptr;
    if (response.token == nullptr || response.Name == nullptr ||
        ((include_video_source || include_video_encoder) && response.Configurations == nullptr)) {
        return SOAP_EOM;
    }

    if (response.Configurations == nullptr) {
        return SOAP_OK;
    }
    response.Configurations->VideoSource = include_video_source ? make_video_source_configuration(ctx, profile) : nullptr;
    response.Configurations->VideoEncoder = include_video_encoder ? make_video_encoder_configuration(ctx, profile) : nullptr;
    if ((include_video_source && response.Configurations->VideoSource == nullptr) ||
        (include_video_encoder && response.Configurations->VideoEncoder == nullptr)) {
        return SOAP_EOM;
    }
    return SOAP_OK;
}

int fill_media2_profiles_response(soap* ctx, _tr2__GetProfilesResponse& response,
                                  const MediaModel& model, const _tr2__GetProfiles* request) {
    std::vector<VideoProfile> profiles;
    const std::string requested_token = request == nullptr ? std::string{} : string_value(request->Token);
    if (requested_token.empty()) {
        profiles = model.profiles;
    } else {
        profiles.push_back(profile_for_token(model, requested_token));
    }
    response.__sizeProfiles = static_cast<int>(profiles.size());
    response.Profiles = soap_make_array<tr2__MediaProfile>(ctx, profiles.size());
    if (!profiles.empty() && response.Profiles == nullptr) {
        return SOAP_EOM;
    }
    for (std::size_t i = 0; i < profiles.size(); ++i) {
        if (const int err = fill_media2_profile(ctx, response.Profiles[i], profiles[i], request); err != SOAP_OK) {
            return err;
        }
    }
    return SOAP_OK;
}

int fill_media2_stream_uri_response(soap* ctx, _tr2__GetStreamUriResponse& response, const MediaModel& model) {
    response.Uri = soap_strdup(ctx, model.rtsp_uri.c_str());
    return response.Uri == nullptr ? SOAP_EOM : SOAP_OK;
}

int fill_media2_service_capabilities_response(soap* ctx, _tr2__GetServiceCapabilitiesResponse& response) {
    response.Capabilities = soap_make<tr2__Capabilities>(ctx);
    if (response.Capabilities == nullptr) {
        return SOAP_EOM;
    }
    response.Capabilities->SnapshotUri = false;
    response.Capabilities->Rotation = false;
    response.Capabilities->VideoSourceMode = false;
    response.Capabilities->OSD = false;
    response.Capabilities->TemporaryOSDText = false;
    response.Capabilities->EXICompression = false;
    return SOAP_OK;
}

}  // namespace
}  // namespace afterveda_onvif

using namespace afterveda_onvif;

int __trt__GetProfiles(soap* ctx, _trt__GetProfiles*, _trt__GetProfilesResponse& response) {
    if (const int err = require_auth(ctx); err != SOAP_OK) return err;
    return fill_media1_profiles_response(ctx, response, media_model_from_config(current_config(ctx)));
}

int __trt__GetProfile(soap* ctx, _trt__GetProfile* request, _trt__GetProfileResponse& response) {
    if (const int err = require_auth(ctx); err != SOAP_OK) return err;
    const MediaModel model = media_model_from_config(current_config(ctx));
    const std::string token = request == nullptr ? std::string{} : string_value(request->ProfileToken);
    if (token.empty()) {
        return soap_sender_fault(ctx, "Missing ProfileToken", nullptr);
    }
    if (!has_profile_token(model, token)) {
        return invalid_token_fault(ctx, token);
    }
    return fill_media1_get_profile_response(ctx, response, profile_for_token(model, token));
}

int __trt__GetStreamUri(soap* ctx, _trt__GetStreamUri* request, _trt__GetStreamUriResponse& response) {
    if (const int err = require_auth(ctx); err != SOAP_OK) return err;
    if (!media1_stream_setup_supported(request)) {
        return soap_sender_fault(ctx, "Only RTP/RTSP/TCP unicast streaming is supported", nullptr);
    }
    const MediaModel model = media_model_from_config(current_config(ctx));
    const std::string token = request == nullptr ? std::string{} : string_value(request->ProfileToken);
    if (token.empty()) {
        return soap_sender_fault(ctx, "Missing ProfileToken", nullptr);
    }
    if (!has_profile_token(model, token)) {
        return invalid_token_fault(ctx, token);
    }
    return fill_media1_stream_uri_response(ctx, response, model);
}

int __trt__GetVideoEncoderConfiguration(soap* ctx, _trt__GetVideoEncoderConfiguration* request, _trt__GetVideoEncoderConfigurationResponse& response) {
    if (const int err = require_auth(ctx); err != SOAP_OK) return err;
    const MediaModel model = media_model_from_config(current_config(ctx));
    const std::string token = request == nullptr ? std::string{} : string_value(request->ConfigurationToken);
    if (token.empty()) {
        return soap_sender_fault(ctx, "Missing ConfigurationToken", nullptr);
    }
    if (!has_profile_token(model, token)) {
        return invalid_token_fault(ctx, token);
    }
    return fill_media1_video_encoder_configuration_response(ctx, response, profile_for_token(model, token));
}

int __trt__SetVideoEncoderConfiguration(soap* ctx, _trt__SetVideoEncoderConfiguration* request, _trt__SetVideoEncoderConfigurationResponse&) {
    if (const int err = require_auth(ctx); err != SOAP_OK) return err;
    const MediaModel model = media_model_from_config(current_config(ctx));
    const std::string token = encoder_request_profile_token(request);
    if (token.empty()) {
        return soap_sender_fault(ctx, "Missing video encoder configuration token", nullptr);
    }
    if (!has_profile_token(model, token)) {
        return invalid_token_fault(ctx, token);
    }
    const auto next = video_config_from_encoder_request(model, request, token);
    if (!next.has_value()) {
        return soap_sender_fault(
            ctx,
            "Unsupported video encoder configuration; use H264, 640x360/1280x720/1920x1080, and 30 fps",
            nullptr);
    }
    const std::string fault = set_video_config(current_config(ctx), *next);
    if (!fault.empty()) {
        return soap_sender_fault(ctx, fault.c_str(), nullptr);
    }
    std::cout << "[info] onvif encoder config selected: "
              << next->token
              << " " << next->width << "x" << next->height
              << " fps=" << next->fps
              << " bitrate=" << next->bitrate_kbps << "\n";
    return SOAP_OK;
}

int __trt__GetServiceCapabilities(soap* ctx, _trt__GetServiceCapabilities*, _trt__GetServiceCapabilitiesResponse& response) {
    if (const int err = require_auth(ctx); err != SOAP_OK) return err;
    return fill_media1_service_capabilities_response(ctx, response);
}

int __trt__GetVideoSources(soap* ctx, _trt__GetVideoSources*, _trt__GetVideoSourcesResponse& response) {
    if (const int err = require_auth(ctx); err != SOAP_OK) return err;
    return fill_media1_video_sources_response(ctx, response, media_model_from_config(current_config(ctx)));
}

int __trt__GetVideoSourceConfigurations(soap* ctx, _trt__GetVideoSourceConfigurations*, _trt__GetVideoSourceConfigurationsResponse& response) {
    if (const int err = require_auth(ctx); err != SOAP_OK) return err;
    return fill_media1_video_source_configurations_response(ctx, response, media_model_from_config(current_config(ctx)));
}

int __trt__GetVideoSourceConfiguration(soap* ctx, _trt__GetVideoSourceConfiguration* request, _trt__GetVideoSourceConfigurationResponse& response) {
    if (const int err = require_auth(ctx); err != SOAP_OK) return err;
    const std::string token = request == nullptr ? std::string{} : string_value(request->ConfigurationToken);
    if (token.empty()) {
        return soap_sender_fault(ctx, "Missing ConfigurationToken", nullptr);
    }
    if (token != kVideoSourceConfigToken) {
        return invalid_token_fault(ctx, token);
    }
    return fill_media1_video_source_configuration_response(ctx, response, media_model_from_config(current_config(ctx)));
}

int __trt__GetVideoEncoderConfigurations(soap* ctx, _trt__GetVideoEncoderConfigurations*, _trt__GetVideoEncoderConfigurationsResponse& response) {
    if (const int err = require_auth(ctx); err != SOAP_OK) return err;
    return fill_media1_video_encoder_configurations_response(ctx, response, media_model_from_config(current_config(ctx)));
}

int __trt__GetVideoSourceConfigurationOptions(soap* ctx, _trt__GetVideoSourceConfigurationOptions* request, _trt__GetVideoSourceConfigurationOptionsResponse& response) {
    if (const int err = require_auth(ctx); err != SOAP_OK) return err;
    const MediaModel model = media_model_from_config(current_config(ctx));
    const std::string configuration_token = request == nullptr ? std::string{} : string_value(request->ConfigurationToken);
    const std::string profile_token = request == nullptr ? std::string{} : string_value(request->ProfileToken);
    if (!configuration_token.empty() && configuration_token != kVideoSourceConfigToken) {
        return invalid_token_fault(ctx, configuration_token);
    }
    if (!profile_token.empty() && !has_profile_token(model, profile_token)) {
        return invalid_token_fault(ctx, profile_token);
    }
    return fill_media1_video_source_configuration_options_response(ctx, response, model);
}

int __trt__GetVideoEncoderConfigurationOptions(soap* ctx, _trt__GetVideoEncoderConfigurationOptions* request, _trt__GetVideoEncoderConfigurationOptionsResponse& response) {
    if (const int err = require_auth(ctx); err != SOAP_OK) return err;
    const MediaModel model = media_model_from_config(current_config(ctx));
    const std::string configuration_token = request == nullptr ? std::string{} : string_value(request->ConfigurationToken);
    const std::string profile_token = request == nullptr ? std::string{} : string_value(request->ProfileToken);
    if (!configuration_token.empty() && !has_profile_token(model, configuration_token)) {
        return invalid_token_fault(ctx, configuration_token);
    }
    if (!profile_token.empty() && !has_profile_token(model, profile_token)) {
        return invalid_token_fault(ctx, profile_token);
    }
    return fill_media1_video_encoder_configuration_options_response(ctx, response, model);
}

int __tr2__GetProfiles(soap* ctx, _tr2__GetProfiles* request, _tr2__GetProfilesResponse& response) {
    if (const int err = require_auth(ctx); err != SOAP_OK) return err;
    const MediaModel model = media_model_from_config(current_config(ctx));
    const std::string token = request == nullptr ? std::string{} : string_value(request->Token);
    if (!token.empty() && !has_profile_token(model, token)) {
        return invalid_token_fault(ctx, token);
    }
    return fill_media2_profiles_response(ctx, response, model, request);
}

int __tr2__GetStreamUri(soap* ctx, _tr2__GetStreamUri* request, _tr2__GetStreamUriResponse& response) {
    if (const int err = require_auth(ctx); err != SOAP_OK) return err;
    if (request == nullptr || !media2_protocol_supported(request->Protocol)) {
        return soap_sender_fault(ctx, "Only RTP/RTSP/TCP unicast streaming is supported", nullptr);
    }
    const MediaModel model = media_model_from_config(current_config(ctx));
    const std::string token = string_value(request->ProfileToken);
    if (token.empty()) {
        return soap_sender_fault(ctx, "Missing ProfileToken", nullptr);
    }
    if (!has_profile_token(model, token)) {
        return invalid_token_fault(ctx, token);
    }
    return fill_media2_stream_uri_response(ctx, response, model);
}

int __tr2__GetServiceCapabilities(soap* ctx, _tr2__GetServiceCapabilities*, _tr2__GetServiceCapabilitiesResponse& response) {
    if (const int err = require_auth(ctx); err != SOAP_OK) return err;
    return fill_media2_service_capabilities_response(ctx, response);
}
