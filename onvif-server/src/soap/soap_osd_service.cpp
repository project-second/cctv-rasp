#include "soap_common.h"

#include "media_model.h"
#include "rail_client.h"

#include <cstddef>
#include <cstring>
#include <string>

namespace afterveda_onvif {
namespace {

constexpr const char* kOsdToken = "osd-main";

template <typename T>
T* soap_make_array(soap* ctx, std::size_t count) {
    if (count == 0) return nullptr;
    void* memory = soap_malloc(ctx, sizeof(T) * count);
    if (!memory) return nullptr;
    T* values = static_cast<T*>(memory);
    for (std::size_t i = 0; i < count; ++i) {
        new (&values[i]) T();
    }
    return values;
}

bool string_equals(const char* value, const char* expected) {
    return value && std::strcmp(value, expected) == 0;
}

int validate_configuration_token(soap* ctx, const char* token, bool required) {
    if (!token || token[0] == '\0') {
        if (!required) return SOAP_OK;
        return soap_sender_fault(ctx, "Missing ConfigurationToken", nullptr);
    }
    if (!string_equals(token, kVideoSourceConfigToken)) {
        return invalid_token_fault(ctx, token);
    }
    return SOAP_OK;
}

int validate_osd_token(soap* ctx, const char* token) {
    if (!token || token[0] == '\0') {
        return soap_sender_fault(ctx, "Missing OSDToken", nullptr);
    }
    if (!string_equals(token, kOsdToken)) {
        return invalid_token_fault(ctx, token);
    }
    return SOAP_OK;
}

int fill_osd(soap* ctx, const OsdSettings& settings, tt__OSDConfiguration& osd) {
    osd.token = soap_strdup(ctx, kOsdToken);
    osd.VideoSourceConfigurationToken = soap_strdup(ctx, kVideoSourceConfigToken);
    osd.Type = soap_strdup(ctx, "Text");
    osd.Position = soap_make<tt__OSDPosConfiguration>(ctx);
    osd.TextString = soap_make<tt__OSDTextConfiguration>(ctx);
    if (!osd.token || !osd.VideoSourceConfigurationToken || !osd.Type ||
        !osd.Position || !osd.TextString) {
        return SOAP_EOM;
    }

    osd.Position->Type = soap_strdup(ctx, "UpperLeft");
    osd.TextString->Type = soap_strdup(ctx, "Plain");
    osd.TextString->PlainText = soap_strdup(ctx, settings.text.c_str());
    if (!osd.Position->Type || !osd.TextString->Type || !osd.TextString->PlainText) {
        return SOAP_EOM;
    }
    return SOAP_OK;
}

int validate_osd_configuration(
    soap* ctx,
    const tt__OSDConfiguration& osd,
    bool require_osd_token,
    bool require_configuration_token) {
    if (require_osd_token) {
        if (const int err = validate_osd_token(ctx, osd.token); err != SOAP_OK) return err;
    }
    if (require_configuration_token &&
        (!osd.VideoSourceConfigurationToken || osd.VideoSourceConfigurationToken[0] == '\0')) {
        return soap_sender_fault(ctx, "Missing VideoSourceConfigurationToken", nullptr);
    }
    if (osd.VideoSourceConfigurationToken &&
        !string_equals(osd.VideoSourceConfigurationToken, kVideoSourceConfigToken)) {
        return invalid_token_fault(ctx, osd.VideoSourceConfigurationToken);
    }
    if (osd.Type && !string_equals(osd.Type, "Text")) {
        return soap_sender_fault(ctx, "Only Text OSD is supported", nullptr);
    }
    if (!osd.TextString || !osd.TextString->PlainText) {
        return soap_sender_fault(ctx, "Missing OSD PlainText", nullptr);
    }
    if (osd.TextString->Type && !string_equals(osd.TextString->Type, "Plain")) {
        return soap_sender_fault(ctx, "Only Plain OSD text is supported", nullptr);
    }
    return SOAP_OK;
}

int set_osd(soap* ctx, bool enabled, const std::string& text) {
    OsdSettings settings = osd_settings_from_rail(current_config(ctx));
    settings.enabled = enabled;
    settings.text = text;
    const std::string error = set_osd_settings(current_config(ctx), settings);
    if (!error.empty()) {
        return soap_sender_fault(ctx, error.c_str(), nullptr);
    }
    return SOAP_OK;
}

int fill_osd_options(soap* ctx, tt__OSDConfigurationOptions& options) {
    options.MaximumNumberOfOSDs = soap_make<tt__MaximumNumberOfOSDs>(ctx);
    options.Type = soap_make_array<char*>(ctx, 1);
    options.PositionOption = soap_make_array<char*>(ctx, 1);
    options.TextOption = soap_make<tt__OSDTextOptions>(ctx);
    if (!options.MaximumNumberOfOSDs || !options.Type ||
        !options.PositionOption || !options.TextOption) {
        return SOAP_EOM;
    }

    options.MaximumNumberOfOSDs->Total = 1;
    options.MaximumNumberOfOSDs->Image = nullptr;
    options.MaximumNumberOfOSDs->PlainText = soap_make<int>(ctx);
    options.MaximumNumberOfOSDs->Date = nullptr;
    options.MaximumNumberOfOSDs->Time = nullptr;
    options.MaximumNumberOfOSDs->DateAndTime = nullptr;
    if (!options.MaximumNumberOfOSDs->PlainText) return SOAP_EOM;
    *options.MaximumNumberOfOSDs->PlainText = 1;

    options.__sizeType = 1;
    options.Type[0] = soap_strdup(ctx, "Text");
    options.__sizePositionOption = 1;
    options.PositionOption[0] = soap_strdup(ctx, "UpperLeft");

    options.TextOption->__sizeType = 1;
    options.TextOption->Type = soap_make_array<char*>(ctx, 1);
    if (!options.Type[0] || !options.PositionOption[0] || !options.TextOption->Type) {
        return SOAP_EOM;
    }
    options.TextOption->Type[0] = soap_strdup(ctx, "Plain");
    return options.TextOption->Type[0] ? SOAP_OK : SOAP_EOM;
}

}  // namespace
}  // namespace afterveda_onvif

using namespace afterveda_onvif;

int __trt__GetOSDs(
    soap* ctx,
    _trt__GetOSDs* request,
    _trt__GetOSDsResponse& response) {
    if (const int err = require_auth(ctx); err != SOAP_OK) return err;
    if (!request) return soap_sender_fault(ctx, "Missing GetOSDs request", nullptr);
    if (const int err = validate_configuration_token(ctx, request->ConfigurationToken, false); err != SOAP_OK) return err;

    const OsdSettings settings = osd_settings_from_rail(current_config(ctx));
    if (!settings.enabled) {
        response.__sizeOSDs = 0;
        response.OSDs = nullptr;
        return SOAP_OK;
    }

    response.__sizeOSDs = 1;
    response.OSDs = soap_make_array<tt__OSDConfiguration>(ctx, 1);
    if (!response.OSDs) return SOAP_EOM;
    return fill_osd(ctx, settings, response.OSDs[0]);
}

int __trt__GetOSD(
    soap* ctx,
    _trt__GetOSD* request,
    _trt__GetOSDResponse& response) {
    if (const int err = require_auth(ctx); err != SOAP_OK) return err;
    if (!request) return soap_sender_fault(ctx, "Missing GetOSD request", nullptr);
    if (const int err = validate_osd_token(ctx, request->OSDToken); err != SOAP_OK) return err;

    const OsdSettings settings = osd_settings_from_rail(current_config(ctx));
    if (!settings.enabled) {
        return invalid_token_fault(ctx, request->OSDToken);
    }

    response.OSD = soap_make<tt__OSDConfiguration>(ctx);
    if (!response.OSD) return SOAP_EOM;
    return fill_osd(ctx, settings, *response.OSD);
}

int __trt__GetOSDOptions(
    soap* ctx,
    _trt__GetOSDOptions* request,
    _trt__GetOSDOptionsResponse& response) {
    if (const int err = require_auth(ctx); err != SOAP_OK) return err;
    if (!request) return soap_sender_fault(ctx, "Missing GetOSDOptions request", nullptr);
    if (const int err = validate_configuration_token(ctx, request->ConfigurationToken, true); err != SOAP_OK) return err;

    response.OSDOptions = soap_make<tt__OSDConfigurationOptions>(ctx);
    if (!response.OSDOptions) return SOAP_EOM;
    return fill_osd_options(ctx, *response.OSDOptions);
}

int __trt__SetOSD(
    soap* ctx,
    _trt__SetOSD* request,
    _trt__SetOSDResponse&) {
    if (const int err = require_auth(ctx); err != SOAP_OK) return err;
    if (!request || !request->OSD) {
        return soap_sender_fault(ctx, "Missing OSD configuration", nullptr);
    }
    if (const int err = validate_osd_configuration(ctx, *request->OSD, true, false); err != SOAP_OK) return err;

    const OsdSettings current = osd_settings_from_rail(current_config(ctx));
    if (!current.enabled) return invalid_token_fault(ctx, request->OSD->token);
    return set_osd(ctx, true, request->OSD->TextString->PlainText);
}

int __trt__CreateOSD(
    soap* ctx,
    _trt__CreateOSD* request,
    _trt__CreateOSDResponse& response) {
    if (const int err = require_auth(ctx); err != SOAP_OK) return err;
    if (!request || !request->OSD) {
        return soap_sender_fault(ctx, "Missing OSD configuration", nullptr);
    }
    if (const int err = validate_osd_configuration(ctx, *request->OSD, false, true); err != SOAP_OK) return err;

    const OsdSettings current = osd_settings_from_rail(current_config(ctx));
    if (current.enabled) {
        return soap_sender_fault(ctx, "Maximum number of OSDs reached", nullptr);
    }
    if (const int err = set_osd(ctx, true, request->OSD->TextString->PlainText); err != SOAP_OK) return err;

    response.OSDToken = soap_strdup(ctx, kOsdToken);
    return response.OSDToken ? SOAP_OK : SOAP_EOM;
}

int __trt__DeleteOSD(
    soap* ctx,
    _trt__DeleteOSD* request,
    _trt__DeleteOSDResponse&) {
    if (const int err = require_auth(ctx); err != SOAP_OK) return err;
    if (!request) return soap_sender_fault(ctx, "Missing DeleteOSD request", nullptr);
    if (const int err = validate_osd_token(ctx, request->OSDToken); err != SOAP_OK) return err;

    const OsdSettings current = osd_settings_from_rail(current_config(ctx));
    if (!current.enabled) return invalid_token_fault(ctx, request->OSDToken);
    return set_osd(ctx, false, current.text);
}
