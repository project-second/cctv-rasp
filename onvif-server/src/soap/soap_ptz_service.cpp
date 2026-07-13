#include "soap_common.h"

#include "media_model.h"
#include "ptz_device.h"
#include "utils.h"

#include <cerrno>
#include <cmath>
#include <cstddef>
#include <cstdlib>
#include <string>

namespace afterveda_onvif {
namespace {

constexpr const char* kPanTiltVelocitySpace = "http://www.onvif.org/ver10/tptz/PanTiltSpaces/VelocityGenericSpace";
constexpr const char* kPanTiltTranslationSpace = "http://www.onvif.org/ver10/tptz/PanTiltSpaces/TranslationGenericSpace";

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

int fill_float_range(soap* ctx, tt__FloatRange*& range, float min, float max) {
    range = soap_make<tt__FloatRange>(ctx);
    if (range == nullptr) {
        return SOAP_EOM;
    }
    range->Min = min;
    range->Max = max;
    return SOAP_OK;
}

int fill_space_2d(soap* ctx, tt__Space2DDescription& space, const char* uri) {
    space.URI = soap_strdup(ctx, uri);
    if (space.URI == nullptr) {
        return SOAP_EOM;
    }
    if (const int err = fill_float_range(ctx, space.XRange, -1.0F, 1.0F); err != SOAP_OK) {
        return err;
    }
    return fill_float_range(ctx, space.YRange, -1.0F, 1.0F);
}

int fill_ptz_spaces(soap* ctx, tt__PTZSpaces& spaces) {
    spaces.__sizeContinuousPanTiltVelocitySpace = 1;
    spaces.ContinuousPanTiltVelocitySpace = soap_make_array<tt__Space2DDescription>(ctx, 1);
    spaces.__sizeRelativePanTiltTranslationSpace = 1;
    spaces.RelativePanTiltTranslationSpace = soap_make_array<tt__Space2DDescription>(ctx, 1);
    if (spaces.ContinuousPanTiltVelocitySpace == nullptr ||
        spaces.RelativePanTiltTranslationSpace == nullptr) {
        return SOAP_EOM;
    }
    if (const int err = fill_space_2d(ctx, spaces.ContinuousPanTiltVelocitySpace[0], kPanTiltVelocitySpace); err != SOAP_OK) {
        return err;
    }
    return fill_space_2d(ctx, spaces.RelativePanTiltTranslationSpace[0], kPanTiltTranslationSpace);
}

int fill_ptz_node(soap* ctx, tt__PTZNode& node) {
    node.token = soap_strdup(ctx, "ptz-node");
    node.Name = soap_strdup(ctx, "PanTilt");
    node.SupportedPTZSpaces = soap_make<tt__PTZSpaces>(ctx);
    node.MaximumNumberOfPresets = 1;
    node.HomeSupported = true;
    if (node.token == nullptr || node.Name == nullptr || node.SupportedPTZSpaces == nullptr) {
        return SOAP_EOM;
    }

    return fill_ptz_spaces(ctx, *node.SupportedPTZSpaces);
}

int fill_ptz_node_response(soap* ctx, _tptz__GetNodesResponse& response) {
    response.__sizePTZNode = 1;
    response.PTZNode = soap_make_array<tt__PTZNode>(ctx, 1);
    if (response.PTZNode == nullptr) {
        return SOAP_EOM;
    }
    return fill_ptz_node(ctx, response.PTZNode[0]);
}

int fill_ptz_configuration(tt__PTZConfiguration& config, soap* ctx) {
    config.token = soap_strdup(ctx, "ptz");
    config.Name = soap_strdup(ctx, "PanTilt");
    config.UseCount = 1;
    config.NodeToken = soap_strdup(ctx, "ptz-node");
    config.DefaultContinuousPanTiltVelocitySpace = soap_strdup(ctx, kPanTiltVelocitySpace);
    config.DefaultRelativePanTiltTranslationSpace = soap_strdup(ctx, kPanTiltTranslationSpace);
    config.DefaultPTZTimeout = soap_strdup(ctx, "PT1S");
    config.PanTiltLimits = nullptr;
    if (config.token == nullptr ||
        config.Name == nullptr ||
        config.NodeToken == nullptr ||
        config.DefaultContinuousPanTiltVelocitySpace == nullptr ||
        config.DefaultRelativePanTiltTranslationSpace == nullptr ||
        config.DefaultPTZTimeout == nullptr) {
        return SOAP_EOM;
    }
    return SOAP_OK;
}

int fill_ptz_configurations_response(soap* ctx, _tptz__GetConfigurationsResponse& response) {
    response.__sizePTZConfiguration = 1;
    response.PTZConfiguration = soap_make_array<tt__PTZConfiguration>(ctx, 1);
    if (response.PTZConfiguration == nullptr) {
        return SOAP_EOM;
    }
    return fill_ptz_configuration(response.PTZConfiguration[0], ctx);
}

int fill_ptz_configuration_options(soap* ctx, tt__PTZConfigurationOptions& options) {
    options.Spaces = soap_make<tt__PTZSpaces>(ctx);
    options.PTZTimeout = soap_make<tt__DurationRange>(ctx);
    if (options.Spaces == nullptr || options.PTZTimeout == nullptr) {
        return SOAP_EOM;
    }
    if (const int err = fill_ptz_spaces(ctx, *options.Spaces); err != SOAP_OK) {
        return err;
    }
    options.PTZTimeout->Min = soap_strdup(ctx, "PT0.1S");
    options.PTZTimeout->Max = soap_strdup(ctx, "PT1H");
    return options.PTZTimeout->Min == nullptr || options.PTZTimeout->Max == nullptr ? SOAP_EOM : SOAP_OK;
}

int fill_ptz_presets_response(soap* ctx, _tptz__GetPresetsResponse& response) {
    response.__sizePreset = 1;
    response.Preset = soap_make_array<tt__PTZPreset>(ctx, 1);
    if (response.Preset == nullptr) {
        return SOAP_EOM;
    }
    response.Preset[0].token = soap_strdup(ctx, "home");
    response.Preset[0].Name = soap_strdup(ctx, "Home");
    return response.Preset[0].token == nullptr || response.Preset[0].Name == nullptr ? SOAP_EOM : SOAP_OK;
}

int fill_ptz_status_response(soap* ctx, _tptz__GetStatusResponse& response, const PtzStatus& status) {
    response.PTZStatus = soap_make<tt__PTZStatus>(ctx);
    if (!response.PTZStatus) return SOAP_EOM;

    response.PTZStatus->Position = soap_make<tt__PTZVector>(ctx);
    response.PTZStatus->MoveStatus = soap_make<tt__PTZMoveStatus>(ctx);
    response.PTZStatus->UtcTime = soap_strdup(ctx, now_utc().c_str());
    if (!response.PTZStatus->Position || !response.PTZStatus->MoveStatus || !response.PTZStatus->UtcTime) return SOAP_EOM;

    response.PTZStatus->Position->PanTilt = soap_make<tt__Vector2D>(ctx);
    response.PTZStatus->Position->Zoom = nullptr;
    if (!response.PTZStatus->Position->PanTilt) return SOAP_EOM;

    response.PTZStatus->Position->PanTilt->x = ptz_pan_to_onvif(status.position.pan);
    response.PTZStatus->Position->PanTilt->y = ptz_tilt_to_onvif(status.position.tilt);
    const bool moving = status.pan_speed != 0 || status.tilt_speed != 0;
    response.PTZStatus->MoveStatus->PanTilt = soap_strdup(ctx, moving ? "MOVING" : "IDLE");
    response.PTZStatus->MoveStatus->Zoom = nullptr;
    if (!response.PTZStatus->MoveStatus->PanTilt) return SOAP_EOM;
    return SOAP_OK;
}

std::string string_value(const char* value) {
    return value == nullptr ? std::string{} : std::string{value};
}

bool is_supported_preset_token(const std::string& token) {
    return token == "home";
}

bool typed_zoom_requested(const tt__Vector1D* zoom) {
    return zoom && std::fabs(zoom->x) > 0.000001;
}

bool parse_ptz_timeout_ms(const char* value, int& timeout_ms) {
    if (value == nullptr || *value == '\0') {
        timeout_ms = 1000;
        return true;
    }
    const std::string duration = value;
    if (duration.rfind("PT", 0) != 0 || duration.size() <= 2) {
        return false;
    }

    const char* cursor = duration.c_str() + 2;
    double seconds = 0.0;
    while (*cursor != '\0') {
        errno = 0;
        char* end = nullptr;
        const double amount = std::strtod(cursor, &end);
        if (end == cursor || errno == ERANGE || amount < 0.0 || *end == '\0') {
            return false;
        }
        switch (*end) {
            case 'H': seconds += amount * 3600.0; break;
            case 'M': seconds += amount * 60.0; break;
            case 'S': seconds += amount; break;
            default: return false;
        }
        cursor = end + 1;
    }
    if (seconds <= 0.0 || seconds > 3600.0) {
        return false;
    }
    timeout_ms = static_cast<int>(std::lround(seconds * 1000.0));
    return timeout_ms > 0;
}

int validate_profile_token(soap* ctx, const char* value) {
    const std::string token = string_value(value);
    if (token.empty()) {
        return soap_sender_fault(ctx, "Missing ProfileToken", nullptr);
    }
    if (!has_profile_token(media_model_from_config(current_config(ctx)), token)) {
        return invalid_token_fault(ctx, token);
    }
    return SOAP_OK;
}

int validate_node_token(soap* ctx, const char* value) {
    const std::string token = string_value(value);
    if (token != "ptz-node") {
        return invalid_token_fault(ctx, token);
    }
    return SOAP_OK;
}

int validate_configuration_token(soap* ctx, const char* value) {
    const std::string token = string_value(value);
    if (token != "ptz") {
        return invalid_token_fault(ctx, token);
    }
    return SOAP_OK;
}

}  // namespace
}  // namespace afterveda_onvif

using namespace afterveda_onvif;

int __tptz__GetServiceCapabilities(
    soap* ctx,
    _tptz__GetServiceCapabilities*,
    _tptz__GetServiceCapabilitiesResponse& response) {
    response.Capabilities = soap_make<tptz__Capabilities>(ctx);
    if (response.Capabilities == nullptr) return SOAP_EOM;
    response.Capabilities->EFlip = false;
    response.Capabilities->Reverse = false;
    response.Capabilities->GetCompatibleConfigurations = false;
    response.Capabilities->MoveStatus = true;
    response.Capabilities->StatusPosition = true;
    return SOAP_OK;
}

int __tptz__GetNodes(soap* ctx, _tptz__GetNodes*, _tptz__GetNodesResponse& response) {
    if (const int err = require_auth(ctx); err != SOAP_OK) return err;
    return fill_ptz_node_response(ctx, response);
}

int __tptz__GetNode(soap* ctx, _tptz__GetNode* request, _tptz__GetNodeResponse& response) {
    if (const int err = require_auth(ctx); err != SOAP_OK) return err;
    if (const int err = validate_node_token(ctx, request == nullptr ? nullptr : request->NodeToken); err != SOAP_OK) return err;
    response.PTZNode = soap_make<tt__PTZNode>(ctx);
    if (response.PTZNode == nullptr) return SOAP_EOM;
    return fill_ptz_node(ctx, *response.PTZNode);
}

int __tptz__GetConfigurations(soap* ctx, _tptz__GetConfigurations*, _tptz__GetConfigurationsResponse& response) {
    if (const int err = require_auth(ctx); err != SOAP_OK) return err;
    return fill_ptz_configurations_response(ctx, response);
}

int __tptz__GetConfiguration(
    soap* ctx,
    _tptz__GetConfiguration* request,
    _tptz__GetConfigurationResponse& response) {
    if (const int err = require_auth(ctx); err != SOAP_OK) return err;
    if (const int err = validate_configuration_token(
            ctx, request == nullptr ? nullptr : request->PTZConfigurationToken); err != SOAP_OK) return err;
    response.PTZConfiguration = soap_make<tt__PTZConfiguration>(ctx);
    if (response.PTZConfiguration == nullptr) return SOAP_EOM;
    return fill_ptz_configuration(*response.PTZConfiguration, ctx);
}

int __tptz__GetConfigurationOptions(
    soap* ctx,
    _tptz__GetConfigurationOptions* request,
    _tptz__GetConfigurationOptionsResponse& response) {
    if (const int err = require_auth(ctx); err != SOAP_OK) return err;
    if (const int err = validate_configuration_token(
            ctx, request == nullptr ? nullptr : request->ConfigurationToken); err != SOAP_OK) return err;
    response.PTZConfigurationOptions = soap_make<tt__PTZConfigurationOptions>(ctx);
    if (response.PTZConfigurationOptions == nullptr) return SOAP_EOM;
    return fill_ptz_configuration_options(ctx, *response.PTZConfigurationOptions);
}

int __tptz__GetPresets(soap* ctx, _tptz__GetPresets* request, _tptz__GetPresetsResponse& response) {
    if (const int err = require_auth(ctx); err != SOAP_OK) return err;
    if (const int err = validate_profile_token(ctx, request == nullptr ? nullptr : request->ProfileToken); err != SOAP_OK) return err;
    return fill_ptz_presets_response(ctx, response);
}

int __tptz__SetPreset(soap* ctx, _tptz__SetPreset* request, _tptz__SetPresetResponse& response) {
    if (const int err = require_auth(ctx); err != SOAP_OK) return err;
    if (const int err = validate_profile_token(ctx, request == nullptr ? nullptr : request->ProfileToken); err != SOAP_OK) return err;
    const std::string token = request == nullptr ? std::string{} : string_value(request->PresetToken);
    if (!token.empty() && !is_supported_preset_token(token)) {
        return invalid_token_fault(ctx, token);
    }
    const std::string fault = set_home_ptz_from_current(current_config(ctx));
    if (!fault.empty()) return soap_sender_fault(ctx, fault.c_str(), nullptr);
    response.PresetToken = soap_strdup(ctx, "home");
    return response.PresetToken == nullptr ? SOAP_EOM : SOAP_OK;
}

int __tptz__GotoPreset(soap* ctx, _tptz__GotoPreset* request, _tptz__GotoPresetResponse&) {
    if (const int err = require_auth(ctx); err != SOAP_OK) return err;
    if (const int err = validate_profile_token(ctx, request == nullptr ? nullptr : request->ProfileToken); err != SOAP_OK) return err;
    const std::string token = request == nullptr ? std::string{} : string_value(request->PresetToken);
    if (!is_supported_preset_token(token)) {
        return invalid_token_fault(ctx, token);
    }
    if (request != nullptr && request->Speed != nullptr && typed_zoom_requested(request->Speed->Zoom)) {
        return soap_sender_fault(ctx, "Zoom is not supported by this device", nullptr);
    }
    const std::string fault = home_ptz(current_config(ctx));
    if (!fault.empty()) return soap_sender_fault(ctx, fault.c_str(), nullptr);
    return SOAP_OK;
}

int __tptz__GetStatus(soap* ctx, _tptz__GetStatus* request, _tptz__GetStatusResponse& response) {
    if (const int err = require_auth(ctx); err != SOAP_OK) return err;
    if (const int err = validate_profile_token(ctx, request == nullptr ? nullptr : request->ProfileToken); err != SOAP_OK) return err;
    PtzStatus status{};
    const std::string fault = read_ptz_status(current_config(ctx), status);
    if (!fault.empty()) return soap_sender_fault(ctx, fault.c_str(), nullptr);
    return fill_ptz_status_response(ctx, response, status);
}

int __tptz__SetHomePosition(soap* ctx, _tptz__SetHomePosition* request, _tptz__SetHomePositionResponse&) {
    if (const int err = require_auth(ctx); err != SOAP_OK) return err;
    if (const int err = validate_profile_token(ctx, request == nullptr ? nullptr : request->ProfileToken); err != SOAP_OK) return err;
    const std::string fault = set_home_ptz_from_current(current_config(ctx));
    if (!fault.empty()) return soap_sender_fault(ctx, fault.c_str(), nullptr);
    return SOAP_OK;
}

int __tptz__GotoHomePosition(soap* ctx, _tptz__GotoHomePosition* request, _tptz__GotoHomePositionResponse&) {
    if (const int err = require_auth(ctx); err != SOAP_OK) return err;
    if (const int err = validate_profile_token(ctx, request == nullptr ? nullptr : request->ProfileToken); err != SOAP_OK) return err;
    if (request != nullptr && request->Speed != nullptr && typed_zoom_requested(request->Speed->Zoom)) {
        return soap_sender_fault(ctx, "Zoom is not supported by this device", nullptr);
    }
    const std::string fault = home_ptz(current_config(ctx));
    if (!fault.empty()) return soap_sender_fault(ctx, fault.c_str(), nullptr);
    return SOAP_OK;
}

int __tptz__Stop(soap* ctx, _tptz__Stop* request, _tptz__StopResponse&) {
    if (const int err = require_auth(ctx); err != SOAP_OK) return err;
    if (const int err = validate_profile_token(ctx, request == nullptr ? nullptr : request->ProfileToken); err != SOAP_OK) return err;
    const bool stop_pan_tilt = request == nullptr || request->PanTilt == nullptr || *request->PanTilt;
    if (stop_pan_tilt) {
        const std::string fault = stop_ptz(current_config(ctx));
        if (!fault.empty()) return soap_sender_fault(ctx, fault.c_str(), nullptr);
    }
    return SOAP_OK;
}

int __tptz__ContinuousMove(soap* ctx, _tptz__ContinuousMove* request, _tptz__ContinuousMoveResponse&) {
    if (const int err = require_auth(ctx); err != SOAP_OK) return err;
    if (const int err = validate_profile_token(ctx, request == nullptr ? nullptr : request->ProfileToken); err != SOAP_OK) return err;
    if (request != nullptr && request->Velocity != nullptr && typed_zoom_requested(request->Velocity->Zoom)) {
        return soap_sender_fault(ctx, "Zoom is not supported by this device", nullptr);
    }
    if (request == nullptr || request->Velocity == nullptr || request->Velocity->PanTilt == nullptr) {
        return soap_sender_fault(ctx, "Missing pan/tilt velocity", nullptr);
    }
    int timeout_ms = 0;
    if (!parse_ptz_timeout_ms(request->Timeout, timeout_ms)) {
        return soap_sender_fault(ctx, "Invalid PTZ Timeout; expected an ISO 8601 duration greater than PT0S and no more than PT1H", nullptr);
    }
    const std::string fault = continuous_move_ptz(
        current_config(ctx), request->Velocity->PanTilt->x, request->Velocity->PanTilt->y, timeout_ms);
    if (!fault.empty()) return soap_sender_fault(ctx, fault.c_str(), nullptr);
    return SOAP_OK;
}

int __tptz__RelativeMove(soap* ctx, _tptz__RelativeMove* request, _tptz__RelativeMoveResponse&) {
    if (const int err = require_auth(ctx); err != SOAP_OK) return err;
    if (const int err = validate_profile_token(ctx, request == nullptr ? nullptr : request->ProfileToken); err != SOAP_OK) return err;
    if (request != nullptr && request->Translation != nullptr && typed_zoom_requested(request->Translation->Zoom)) {
        return soap_sender_fault(ctx, "Zoom is not supported by this device", nullptr);
    }
    if (request == nullptr || request->Translation == nullptr || request->Translation->PanTilt == nullptr) {
        return soap_sender_fault(ctx, "Missing pan/tilt translation", nullptr);
    }
    const std::string fault = relative_move_ptz(
        current_config(ctx), request->Translation->PanTilt->x, request->Translation->PanTilt->y);
    if (!fault.empty()) return soap_sender_fault(ctx, fault.c_str(), nullptr);
    return SOAP_OK;
}
