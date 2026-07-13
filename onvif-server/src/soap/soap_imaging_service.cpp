#include "soap_common.h"

#include "media_model.h"
#include "rail_client.h"

#include <cmath>
#include <string>

namespace afterveda_onvif {
namespace {

int validate_video_source_token(soap* ctx, const char* token) {
    if (!token || token[0] == '\0') {
        return soap_sender_fault(ctx, "Missing VideoSourceToken", nullptr);
    }
    if (std::string(token) != kVideoSourceToken) {
        return invalid_token_fault(ctx, token);
    }
    return SOAP_OK;
}

float* make_float(soap* ctx, double value) {
    float* result = soap_make<float>(ctx);
    if (result) {
        *result = static_cast<float>(value);
    }
    return result;
}

tt__FloatRange* make_range(soap* ctx, float minimum, float maximum) {
    tt__FloatRange* range = soap_make<tt__FloatRange>(ctx);
    if (range) {
        range->Min = minimum;
        range->Max = maximum;
    }
    return range;
}

bool valid_setting(const float* value) {
    return !value || (std::isfinite(*value) && *value >= 0.0F && *value <= 100.0F);
}

int fill_settings_response(
    soap* ctx,
    const ImageSettings& settings,
    _timg__GetImagingSettingsResponse& response) {
    response.ImagingSettings = soap_make<tt__ImagingSettings20>(ctx);
    if (!response.ImagingSettings) return SOAP_EOM;

    response.ImagingSettings->Brightness = make_float(ctx, settings.brightness);
    response.ImagingSettings->Contrast = make_float(ctx, settings.contrast);
    response.ImagingSettings->ColorSaturation = make_float(ctx, settings.color_saturation);
    if (!response.ImagingSettings->Brightness ||
        !response.ImagingSettings->Contrast ||
        !response.ImagingSettings->ColorSaturation) {
        return SOAP_EOM;
    }
    return SOAP_OK;
}

int fill_options_response(soap* ctx, _timg__GetOptionsResponse& response) {
    response.ImagingOptions = soap_make<tt__ImagingOptions20>(ctx);
    if (!response.ImagingOptions) return SOAP_EOM;

    response.ImagingOptions->Brightness = make_range(ctx, 0.0F, 100.0F);
    response.ImagingOptions->Contrast = make_range(ctx, 0.0F, 100.0F);
    response.ImagingOptions->ColorSaturation = make_range(ctx, 0.0F, 100.0F);
    if (!response.ImagingOptions->Brightness ||
        !response.ImagingOptions->Contrast ||
        !response.ImagingOptions->ColorSaturation) {
        return SOAP_EOM;
    }
    return SOAP_OK;
}

}  // namespace
}  // namespace afterveda_onvif

using namespace afterveda_onvif;

int __timg__GetOptions(
    soap* ctx,
    _timg__GetOptions* request,
    _timg__GetOptionsResponse& response) {
    if (const int err = require_auth(ctx); err != SOAP_OK) return err;
    if (!request) return soap_sender_fault(ctx, "Missing GetOptions request", nullptr);
    if (const int err = validate_video_source_token(ctx, request->VideoSourceToken); err != SOAP_OK) return err;
    return fill_options_response(ctx, response);
}

int __timg__GetImagingSettings(
    soap* ctx,
    _timg__GetImagingSettings* request,
    _timg__GetImagingSettingsResponse& response) {
    if (const int err = require_auth(ctx); err != SOAP_OK) return err;
    if (!request) return soap_sender_fault(ctx, "Missing GetImagingSettings request", nullptr);
    if (const int err = validate_video_source_token(ctx, request->VideoSourceToken); err != SOAP_OK) return err;
    return fill_settings_response(ctx, imaging_settings_from_rail(current_config(ctx)), response);
}

int __timg__SetImagingSettings(
    soap* ctx,
    _timg__SetImagingSettings* request,
    _timg__SetImagingSettingsResponse&) {
    if (const int err = require_auth(ctx); err != SOAP_OK) return err;
    if (!request) return soap_sender_fault(ctx, "Missing SetImagingSettings request", nullptr);
    if (const int err = validate_video_source_token(ctx, request->VideoSourceToken); err != SOAP_OK) return err;
    if (!request->ImagingSettings) {
        return soap_sender_fault(ctx, "Missing ImagingSettings", nullptr);
    }

    const tt__ImagingSettings20& input = *request->ImagingSettings;
    if (!valid_setting(input.Brightness)) {
        return soap_sender_fault(ctx, "Invalid Brightness value", nullptr);
    }
    if (!valid_setting(input.Contrast)) {
        return soap_sender_fault(ctx, "Invalid Contrast value", nullptr);
    }
    if (!valid_setting(input.ColorSaturation)) {
        return soap_sender_fault(ctx, "Invalid ColorSaturation value", nullptr);
    }
    if (!input.Brightness && !input.Contrast && !input.ColorSaturation) {
        return soap_sender_fault(ctx, "No supported imaging settings were provided", nullptr);
    }

    ImageSettings settings = imaging_settings_from_rail(current_config(ctx));
    if (input.Brightness) settings.brightness = *input.Brightness;
    if (input.Contrast) settings.contrast = *input.Contrast;
    if (input.ColorSaturation) settings.color_saturation = *input.ColorSaturation;

    const std::string fault = set_imaging_settings(current_config(ctx), settings);
    if (!fault.empty()) {
        return soap_sender_fault(ctx, fault.c_str(), nullptr);
    }
    return SOAP_OK;
}

int __timg__GetServiceCapabilities(
    soap* ctx,
    _timg__GetServiceCapabilities*,
    _timg__GetServiceCapabilitiesResponse& response) {
    if (const int err = require_auth(ctx); err != SOAP_OK) return err;

    response.Capabilities = soap_make<timg__Capabilities>(ctx);
    if (!response.Capabilities) return SOAP_EOM;
    response.Capabilities->ImageStabilization = false;
    response.Capabilities->Presets = false;
    return SOAP_OK;
}
