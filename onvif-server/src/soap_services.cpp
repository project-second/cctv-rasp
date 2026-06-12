#include "config.h"
#include "media_model.h"
#include "media_xml.h"
#include "ptz_device.h"
#include "rail_client.h"
#include "utils.h"

#include "soapH.h"
#include "DeviceBinding.nsmap"

#include <ctime>
#include <string>
#include <vector>

namespace afterveda_onvif {
namespace {

Config& current_config(soap* ctx) {
    return *static_cast<Config*>(ctx->user);
}

char* soap_xml(soap* ctx, const std::string& value) {
    return soap_strdup(ctx, value.c_str());
}

int require_auth(soap* ctx) {
    const Config& config = current_config(ctx);
    if (config.username.empty() && config.password.empty()) {
        return SOAP_OK;
    }
    if (!ctx->header || !ctx->header->wsse__Security || !ctx->header->wsse__Security->UsernameToken) {
        return soap_sender_fault(ctx, "ONVIF authentication required", nullptr);
    }
    const _wsse__UsernameToken* token = ctx->header->wsse__Security->UsernameToken;
    if (!token->Username || config.username != token->Username || !token->Password || !token->Password->__item) {
        return soap_sender_fault(ctx, "ONVIF authentication failed", nullptr);
    }
    const std::string password = token->Password->__item;
    const std::string type = token->Password->Type ? token->Password->Type : "";
    if (contains(type, "PasswordDigest")) {
        if (!token->Nonce || !token->Nonce->__item || !token->wsu__Created || !utc_timestamp_within(token->wsu__Created, 300)) {
            return soap_sender_fault(ctx, "ONVIF authentication failed", nullptr);
        }
        std::vector<unsigned char> digest_input = base64_decode(token->Nonce->__item);
        if (digest_input.empty()) {
            return soap_sender_fault(ctx, "ONVIF authentication failed", nullptr);
        }
        digest_input.insert(digest_input.end(), token->wsu__Created, token->wsu__Created + std::char_traits<char>::length(token->wsu__Created));
        digest_input.insert(digest_input.end(), config.password.begin(), config.password.end());
        if (base64_encode(sha1(digest_input)) != password) {
            return soap_sender_fault(ctx, "ONVIF authentication failed", nullptr);
        }
        return SOAP_OK;
    }
    if (password != config.password) {
        return soap_sender_fault(ctx, "ONVIF authentication failed", nullptr);
    }
    return SOAP_OK;
}

std::string ptz_command(const Config& config, const std::string& command) {
    return write_ptz_command(config, command);
}

std::string service_entry(const std::string& ns, const std::string& xaddr, int major, int minor) {
    return "<tds:Service xmlns:tt=\"http://www.onvif.org/ver10/schema\"><tds:Namespace>" + ns + "</tds:Namespace><tds:XAddr>" + xml_escape(xaddr) +
        "</tds:XAddr><tds:Version><tt:Major>" + std::to_string(major) + "</tt:Major><tt:Minor>" +
        std::to_string(minor) + "</tt:Minor></tds:Version></tds:Service>";
}

std::string move_command_for_request(const std::string& request) {
    const bool negative_x = contains(request, "x=\"-") || contains(request, "<tt:x>-");
    const bool positive_x = contains(request, "x=\"0.") || contains(request, "x=\"1") || contains(request, "<tt:x>0.") || contains(request, "<tt:x>1");
    const bool negative_y = contains(request, "y=\"-") || contains(request, "<tt:y>-");
    const bool positive_y = contains(request, "y=\"0.") || contains(request, "y=\"1") || contains(request, "<tt:y>0.") || contains(request, "<tt:y>1");
    if (negative_x) {
        return "left";
    }
    if (positive_x) {
        return "right";
    }
    if (negative_y) {
        return "up";
    }
    if (positive_y) {
        return "down";
    }
    return "stop";
}

bool requests_multicast_or_udp(const std::string& request) {
    return contains(request, "<tt:Stream>RTP-Multicast</tt:Stream") ||
        contains(request, "<Stream>RTP-Multicast</Stream") ||
        contains(request, "<tt:Protocol>UDP</tt:Protocol>") ||
        contains(request, "<Protocol>UDP</Protocol>");
}

int invalid_token_fault(soap* ctx, const std::string& token) {
    return soap_sender_fault(ctx, ("Invalid or unsupported token: " + token).c_str(), nullptr);
}

}  // namespace
}  // namespace afterveda_onvif

using namespace afterveda_onvif;

int __tds__GetCapabilities(soap* ctx, char*, char*& response) {
    if (const int err = require_auth(ctx); err != SOAP_OK) return err;
    response = soap_xml(ctx, device_capabilities_xml(current_config(ctx)));
    return SOAP_OK;
}

int __tds__GetServices(soap* ctx, char*, char*& response) {
    if (const int err = require_auth(ctx); err != SOAP_OK) return err;
    const Config& config = current_config(ctx);
    const std::string base = xaddr_base(config);
    response = soap_xml(ctx,
        service_entry("http://www.onvif.org/ver10/device/wsdl", base + "/device_service", 2, 20) +
        service_entry("http://www.onvif.org/ver10/media/wsdl", base + "/media_service", 2, 20) +
        service_entry("http://www.onvif.org/ver20/media/wsdl", base + "/media2_service", 2, 20) +
        service_entry("http://www.onvif.org/ver20/ptz/wsdl", base + "/ptz_service", 2, 20) +
        service_entry("http://www.onvif.org/ver20/imaging/wsdl", base + "/imaging_service", 2, 20) +
        service_entry("http://www.onvif.org/ver10/events/wsdl", base + "/events_service", 2, 20) +
        service_entry("http://www.onvif.org/ver10/advancedsecurity/wsdl", base + "/device_service", 1, 0));
    return SOAP_OK;
}

int __tds__GetDeviceInformation(soap* ctx, char*, char*& response) {
    if (const int err = require_auth(ctx); err != SOAP_OK) return err;
    const Config& c = current_config(ctx);
    response = soap_xml(ctx,
        "<tds:Manufacturer>" + xml_escape(c.manufacturer) + "</tds:Manufacturer>"
        "<tds:Model>" + xml_escape(c.model) + "</tds:Model>"
        "<tds:FirmwareVersion>" + xml_escape(c.firmware) + "</tds:FirmwareVersion>"
        "<tds:SerialNumber>" + xml_escape(c.serial) + "</tds:SerialNumber>"
        "<tds:HardwareId>" + xml_escape(c.hardware_id) + "</tds:HardwareId>");
    return SOAP_OK;
}

int __tds__GetSystemDateAndTime(soap* ctx, char*, char*& response) {
    if (const int err = require_auth(ctx); err != SOAP_OK) return err;
    std::time_t raw = std::time(nullptr);
    std::tm tm{};
    gmtime_r(&raw, &tm);
    response = soap_xml(ctx,
        "<tds:SystemDateAndTime xmlns:tt=\"http://www.onvif.org/ver10/schema\">"
        "<tt:DateTimeType>NTP</tt:DateTimeType><tt:DaylightSavings>false</tt:DaylightSavings>"
        "<tt:UTCDateTime><tt:Time><tt:Hour>" + std::to_string(tm.tm_hour) + "</tt:Hour><tt:Minute>" + std::to_string(tm.tm_min) + "</tt:Minute><tt:Second>" + std::to_string(tm.tm_sec) + "</tt:Second></tt:Time>"
        "<tt:Date><tt:Year>" + std::to_string(tm.tm_year + 1900) + "</tt:Year><tt:Month>" + std::to_string(tm.tm_mon + 1) + "</tt:Month><tt:Day>" + std::to_string(tm.tm_mday) + "</tt:Day></tt:Date></tt:UTCDateTime>"
        "</tds:SystemDateAndTime>");
    return SOAP_OK;
}

int __tds__GetScopes(soap* ctx, char*, char*& response) {
    if (const int err = require_auth(ctx); err != SOAP_OK) return err;
    const Config& c = current_config(ctx);
    response = soap_xml(ctx,
        "<tds:Scopes xmlns:tt=\"http://www.onvif.org/ver10/schema\"><tt:ScopeDef>Fixed</tt:ScopeDef><tt:ScopeItem>onvif://www.onvif.org/name/" + xml_escape(c.device_name) + "</tt:ScopeItem></tds:Scopes>"
        "<tds:Scopes xmlns:tt=\"http://www.onvif.org/ver10/schema\"><tt:ScopeDef>Fixed</tt:ScopeDef><tt:ScopeItem>onvif://www.onvif.org/hardware/" + xml_escape(c.hardware_id) + "</tt:ScopeItem></tds:Scopes>"
        "<tds:Scopes xmlns:tt=\"http://www.onvif.org/ver10/schema\"><tt:ScopeDef>Fixed</tt:ScopeDef><tt:ScopeItem>onvif://www.onvif.org/Profile/T</tt:ScopeItem></tds:Scopes>");
    return SOAP_OK;
}

int __tds__GetHostname(soap* ctx, char*, char*& response) {
    if (const int err = require_auth(ctx); err != SOAP_OK) return err;
    response = soap_xml(ctx, "<tds:HostnameInformation xmlns:tt=\"http://www.onvif.org/ver10/schema\"><tt:FromDHCP>false</tt:FromDHCP><tt:Name>" + xml_escape(current_config(ctx).device_name) + "</tt:Name></tds:HostnameInformation>");
    return SOAP_OK;
}

int __tds__GetNetworkInterfaces(soap* ctx, char*, char*& response) {
    if (const int err = require_auth(ctx); err != SOAP_OK) return err;
    response = soap_xml(ctx, "<tds:NetworkInterfaces xmlns:tt=\"http://www.onvif.org/ver10/schema\" token=\"eth0\" enabled=\"true\"><tt:Info><tt:Name>eth0</tt:Name><tt:HwAddress>00:00:00:00:00:00</tt:HwAddress><tt:MTU>1500</tt:MTU></tt:Info><tt:IPv4><tt:Enabled>true</tt:Enabled><tt:Config><tt:DHCP>true</tt:DHCP></tt:Config></tt:IPv4></tds:NetworkInterfaces>");
    return SOAP_OK;
}

int __tds__GetUsers(soap* ctx, char*, char*& response) {
    if (const int err = require_auth(ctx); err != SOAP_OK) return err;
    const Config& c = current_config(ctx);
    response = soap_xml(ctx, c.username.empty() ? "" : "<tds:User xmlns:tt=\"http://www.onvif.org/ver10/schema\"><tt:Username>" + xml_escape(c.username) + "</tt:Username><tt:UserLevel>Administrator</tt:UserLevel></tds:User>");
    return SOAP_OK;
}

int __tds__GetServiceCapabilities(soap* ctx, char*, char*& response) {
    if (const int err = require_auth(ctx); err != SOAP_OK) return err;
    response = soap_xml(ctx, "<tds:Capabilities><tds:Network IPFilter=\"false\" ZeroConfiguration=\"false\" IPVersion6=\"false\" DynDNS=\"false\"/><tds:Security TLS1.1=\"false\" TLS1.2=\"false\" OnboardKeyGeneration=\"false\" AccessPolicyConfig=\"false\" DefaultAccessPolicy=\"false\" Dot1X=\"false\" RemoteUserHandling=\"false\"/><tds:System DiscoveryResolve=\"true\" DiscoveryBye=\"false\" RemoteDiscovery=\"false\" SystemBackup=\"false\" SystemLogging=\"false\" FirmwareUpgrade=\"false\"/></tds:Capabilities>");
    return SOAP_OK;
}

int __trt__GetProfiles(soap* ctx, char*, char*& response) {
    if (const int err = require_auth(ctx); err != SOAP_OK) return err;
    response = soap_xml(ctx, media1_profiles_xml(media_model_from_config(current_config(ctx))));
    return SOAP_OK;
}

int __trt__GetStreamUri(soap* ctx, char* request, char*& response) {
    if (const int err = require_auth(ctx); err != SOAP_OK) return err;
    const std::string body = request ? request : "";
    if (requests_multicast_or_udp(body)) {
        return soap_sender_fault(ctx, "Only RTP/RTSP/TCP unicast streaming is supported", nullptr);
    }
    const MediaModel model = media_model_from_config(current_config(ctx));
    const std::string token = extract_profile_token(body);
    if (!token.empty() && !has_profile_token(model, token)) {
        return invalid_token_fault(ctx, token);
    }
    response = soap_xml(ctx, media1_stream_uri_xml(model));
    return SOAP_OK;
}

int __trt__GetVideoEncoderConfiguration(soap* ctx, char* request, char*& response) {
    if (const int err = require_auth(ctx); err != SOAP_OK) return err;
    const MediaModel model = media_model_from_config(current_config(ctx));
    const std::string token = extract_profile_token(request ? request : "");
    if (!token.empty() && !has_profile_token(model, token)) {
        return invalid_token_fault(ctx, token);
    }
    response = soap_xml(ctx, media1_video_encoder_configuration_xml(profile_for_token(model, token)));
    return SOAP_OK;
}

int __trt__SetVideoEncoderConfiguration(soap* ctx, char* request, char*& response) {
    if (const int err = require_auth(ctx); err != SOAP_OK) return err;
    switch_rail_profile(current_config(ctx), extract_profile_token(request ? request : ""));
    response = soap_xml(ctx, "");
    return SOAP_OK;
}

int __trt__GetServiceCapabilities(soap* ctx, char*, char*& response) {
    if (const int err = require_auth(ctx); err != SOAP_OK) return err;
    response = soap_xml(ctx, media1_service_capabilities_xml());
    return SOAP_OK;
}

int __trt__GetVideoSources(soap* ctx, char*, char*& response) {
    if (const int err = require_auth(ctx); err != SOAP_OK) return err;
    response = soap_xml(ctx, media1_video_sources_xml(media_model_from_config(current_config(ctx))));
    return SOAP_OK;
}

int __trt__GetVideoSourceConfigurations(soap* ctx, char*, char*& response) {
    if (const int err = require_auth(ctx); err != SOAP_OK) return err;
    response = soap_xml(ctx, media1_video_source_configurations_xml(media_model_from_config(current_config(ctx))));
    return SOAP_OK;
}

int __trt__GetVideoSourceConfiguration(soap* ctx, char* request, char*& response) {
    if (const int err = require_auth(ctx); err != SOAP_OK) return err;
    const std::string token = first_nonempty({
        xml_text_for_local_name(request ? request : "", "ConfigurationToken"),
        xml_text_for_local_name(request ? request : "", "VideoSourceConfigurationToken"),
    });
    if (!token.empty() && token != kVideoSourceConfigToken) {
        return invalid_token_fault(ctx, token);
    }
    response = soap_xml(ctx, media1_video_source_configuration_xml(media_model_from_config(current_config(ctx))));
    return SOAP_OK;
}

int __trt__GetVideoEncoderConfigurations(soap* ctx, char*, char*& response) {
    if (const int err = require_auth(ctx); err != SOAP_OK) return err;
    response = soap_xml(ctx, media1_video_encoder_configurations_xml(media_model_from_config(current_config(ctx))));
    return SOAP_OK;
}

int __trt__GetVideoSourceConfigurationOptions(soap* ctx, char*, char*& response) {
    if (const int err = require_auth(ctx); err != SOAP_OK) return err;
    response = soap_xml(ctx, media1_video_source_configuration_options_xml(media_model_from_config(current_config(ctx))));
    return SOAP_OK;
}

int __trt__GetVideoEncoderConfigurationOptions(soap* ctx, char* request, char*& response) {
    if (const int err = require_auth(ctx); err != SOAP_OK) return err;
    const MediaModel model = media_model_from_config(current_config(ctx));
    const std::string token = extract_profile_token(request ? request : "");
    if (!token.empty() && !has_profile_token(model, token)) {
        return invalid_token_fault(ctx, token);
    }
    response = soap_xml(ctx, media1_video_encoder_configuration_options_xml(model));
    return SOAP_OK;
}

int __tr2__GetProfiles(soap* ctx, char*, char*& response) {
    if (const int err = require_auth(ctx); err != SOAP_OK) return err;
    response = soap_xml(ctx, media2_profiles_xml(media_model_from_config(current_config(ctx))));
    return SOAP_OK;
}

int __tr2__GetStreamUri(soap* ctx, char* request, char*& response) {
    if (const int err = require_auth(ctx); err != SOAP_OK) return err;
    const std::string body = request ? request : "";
    if (requests_multicast_or_udp(body)) {
        return soap_sender_fault(ctx, "Only RTP/RTSP/TCP unicast streaming is supported", nullptr);
    }
    const MediaModel model = media_model_from_config(current_config(ctx));
    const std::string token = extract_profile_token(body);
    if (!token.empty() && !has_profile_token(model, token)) {
        return invalid_token_fault(ctx, token);
    }
    response = soap_xml(ctx, media2_stream_uri_xml(model));
    return SOAP_OK;
}

int __tr2__GetServiceCapabilities(soap* ctx, char*, char*& response) {
    if (const int err = require_auth(ctx); err != SOAP_OK) return err;
    response = soap_xml(ctx, media2_service_capabilities_xml());
    return SOAP_OK;
}

int __tptz__GetNodes(soap* ctx, char*, char*& response) {
    if (const int err = require_auth(ctx); err != SOAP_OK) return err;
    response = soap_xml(ctx, "<tptz:PTZNode xmlns:tt=\"http://www.onvif.org/ver10/schema\" token=\"ptz-node\"><tt:Name>PanTilt</tt:Name><tt:SupportedPTZSpaces><tt:ContinuousPanTiltVelocitySpace><tt:URI>http://www.onvif.org/ver10/tptz/PanTiltSpaces/VelocityGenericSpace</tt:URI><tt:XRange><tt:Min>-1</tt:Min><tt:Max>1</tt:Max></tt:XRange><tt:YRange><tt:Min>-1</tt:Min><tt:Max>1</tt:Max></tt:YRange></tt:ContinuousPanTiltVelocitySpace><tt:RelativePanTiltTranslationSpace><tt:URI>http://www.onvif.org/ver10/tptz/PanTiltSpaces/TranslationGenericSpace</tt:URI><tt:XRange><tt:Min>-1</tt:Min><tt:Max>1</tt:Max></tt:XRange><tt:YRange><tt:Min>-1</tt:Min><tt:Max>1</tt:Max></tt:YRange></tt:RelativePanTiltTranslationSpace></tt:SupportedPTZSpaces><tt:MaximumNumberOfPresets>1</tt:MaximumNumberOfPresets><tt:HomeSupported>true</tt:HomeSupported></tptz:PTZNode>");
    return SOAP_OK;
}

int __tptz__GetConfigurations(soap* ctx, char*, char*& response) {
    if (const int err = require_auth(ctx); err != SOAP_OK) return err;
    response = soap_xml(ctx, "<tptz:PTZConfiguration xmlns:tt=\"http://www.onvif.org/ver10/schema\" token=\"ptz\"><tt:Name>PanTilt</tt:Name><tt:UseCount>1</tt:UseCount><tt:NodeToken>ptz-node</tt:NodeToken><tt:DefaultContinuousPanTiltVelocitySpace>http://www.onvif.org/ver10/tptz/PanTiltSpaces/VelocityGenericSpace</tt:DefaultContinuousPanTiltVelocitySpace><tt:DefaultRelativePanTiltTranslationSpace>http://www.onvif.org/ver10/tptz/PanTiltSpaces/TranslationGenericSpace</tt:DefaultRelativePanTiltTranslationSpace><tt:DefaultPTZTimeout>PT1S</tt:DefaultPTZTimeout><tt:PanTiltLimits><tt:Range><tt:URI>http://www.onvif.org/ver10/tptz/PanTiltSpaces/PositionGenericSpace</tt:URI><tt:XRange><tt:Min>-1</tt:Min><tt:Max>1</tt:Max></tt:XRange><tt:YRange><tt:Min>-1</tt:Min><tt:Max>1</tt:Max></tt:YRange></tt:Range></tt:PanTiltLimits></tptz:PTZConfiguration>");
    return SOAP_OK;
}

int __tptz__GetPresets(soap* ctx, char*, char*& response) {
    if (const int err = require_auth(ctx); err != SOAP_OK) return err;
    response = soap_xml(ctx, "<tptz:Preset xmlns:tt=\"http://www.onvif.org/ver10/schema\" token=\"home\"><tt:Name>Home</tt:Name></tptz:Preset>");
    return SOAP_OK;
}

int __tptz__SetPreset(soap* ctx, char*, char*& response) {
    if (const int err = require_auth(ctx); err != SOAP_OK) return err;
    response = soap_xml(ctx, "<tptz:PresetToken>home</tptz:PresetToken>");
    return SOAP_OK;
}

int __tptz__GotoPreset(soap* ctx, char*, char*& response) {
    if (const int err = require_auth(ctx); err != SOAP_OK) return err;
    const std::string fault = ptz_command(current_config(ctx), "center");
    if (!fault.empty()) return soap_sender_fault(ctx, fault.c_str(), nullptr);
    response = soap_xml(ctx, "");
    return SOAP_OK;
}

int __tptz__GetStatus(soap* ctx, char*, char*& response) {
    if (const int err = require_auth(ctx); err != SOAP_OK) return err;
    response = soap_xml(ctx, "<tptz:PTZStatus xmlns:tt=\"http://www.onvif.org/ver10/schema\"><tt:Position><tt:PanTilt x=\"0\" y=\"0\"/><tt:Zoom x=\"0\"/></tt:Position><tt:MoveStatus><tt:PanTilt>IDLE</tt:PanTilt><tt:Zoom>IDLE</tt:Zoom></tt:MoveStatus><tt:UtcTime>" + now_utc() + "</tt:UtcTime></tptz:PTZStatus>");
    return SOAP_OK;
}

int __tptz__SetHomePosition(soap* ctx, char*, char*& response) {
    if (const int err = require_auth(ctx); err != SOAP_OK) return err;
    const std::string fault = ptz_command(current_config(ctx), "center");
    if (!fault.empty()) return soap_sender_fault(ctx, fault.c_str(), nullptr);
    response = soap_xml(ctx, "");
    return SOAP_OK;
}

int __tptz__GotoHomePosition(soap* ctx, char*, char*& response) {
    return __tptz__SetHomePosition(ctx, nullptr, response);
}

int __tptz__Stop(soap* ctx, char*, char*& response) {
    if (const int err = require_auth(ctx); err != SOAP_OK) return err;
    const std::string fault = ptz_command(current_config(ctx), "stop");
    if (!fault.empty()) return soap_sender_fault(ctx, fault.c_str(), nullptr);
    response = soap_xml(ctx, "");
    return SOAP_OK;
}

int __tptz__ContinuousMove(soap* ctx, char* request, char*& response) {
    if (const int err = require_auth(ctx); err != SOAP_OK) return err;
    const std::string body = request ? request : "";
    if (contains(body, "Zoom") && !contains(body, "<tt:Zoom x=\"0") && !contains(body, "<Zoom x=\"0")) {
        return soap_sender_fault(ctx, "Zoom is not supported by this device", nullptr);
    }
    const std::string fault = ptz_command(current_config(ctx), move_command_for_request(body));
    if (!fault.empty()) return soap_sender_fault(ctx, fault.c_str(), nullptr);
    response = soap_xml(ctx, "");
    return SOAP_OK;
}

int __tptz__RelativeMove(soap* ctx, char* request, char*& response) {
    return __tptz__ContinuousMove(ctx, request, response);
}

int __timg__GetOptions(soap* ctx, char*, char*& response) {
    if (const int err = require_auth(ctx); err != SOAP_OK) return err;
    response = soap_xml(ctx, "<timg:ImagingOptions xmlns:tt=\"http://www.onvif.org/ver10/schema\"><tt:Brightness><tt:Min>0</tt:Min><tt:Max>100</tt:Max></tt:Brightness><tt:Contrast><tt:Min>0</tt:Min><tt:Max>100</tt:Max></tt:Contrast><tt:ColorSaturation><tt:Min>0</tt:Min><tt:Max>100</tt:Max></tt:ColorSaturation></timg:ImagingOptions>");
    return SOAP_OK;
}

int __timg__GetImagingSettings(soap* ctx, char*, char*& response) {
    if (const int err = require_auth(ctx); err != SOAP_OK) return err;
    response = soap_xml(ctx, "<timg:ImagingSettings xmlns:tt=\"http://www.onvif.org/ver10/schema\"><tt:Brightness>50</tt:Brightness><tt:Contrast>50</tt:Contrast><tt:ColorSaturation>50</tt:ColorSaturation></timg:ImagingSettings>");
    return SOAP_OK;
}

int __timg__SetImagingSettings(soap* ctx, char*, char*& response) {
    if (const int err = require_auth(ctx); err != SOAP_OK) return err;
    response = soap_xml(ctx, "");
    return SOAP_OK;
}

int __timg__GetServiceCapabilities(soap* ctx, char*, char*& response) {
    if (const int err = require_auth(ctx); err != SOAP_OK) return err;
    response = soap_xml(ctx, "<timg:Capabilities ImageStabilization=\"false\" Presets=\"false\"/>");
    return SOAP_OK;
}

int __tev__GetEventProperties(soap* ctx, char*, char*& response) {
    if (const int err = require_auth(ctx); err != SOAP_OK) return err;
    response = soap_xml(ctx, "<tev:TopicNamespaceLocation>http://www.onvif.org/onvif/ver10/topics/topicns.xml</tev:TopicNamespaceLocation><wsnt:FixedTopicSet>true</wsnt:FixedTopicSet><tev:TopicSet xmlns:tns1=\"http://www.onvif.org/ver10/topics\" xmlns:wstop=\"http://docs.oasis-open.org/wsn/t-1\" xmlns:tt=\"http://www.onvif.org/ver10/schema\" xmlns:xs=\"http://www.w3.org/2001/XMLSchema\"><tns1:VideoSource><MotionAlarm wstop:topic=\"true\"><tt:MessageDescription IsProperty=\"true\"><tt:Source><tt:SimpleItemDescription Name=\"VideoSourceConfigurationToken\" Type=\"tt:ReferenceToken\"/></tt:Source><tt:Data><tt:SimpleItemDescription Name=\"State\" Type=\"xs:boolean\"/></tt:Data></tt:MessageDescription></MotionAlarm></tns1:VideoSource></tev:TopicSet><tev:TopicExpressionDialect>http://www.onvif.org/ver10/tev/topicExpression/ConcreteSet</tev:TopicExpressionDialect><tev:MessageContentFilterDialect>http://www.onvif.org/ver10/tev/messageContentFilter/ItemFilter</tev:MessageContentFilterDialect><tev:ProducerPropertiesFilterDialect>http://www.onvif.org/ver10/tev/messageContentFilter/ItemFilter</tev:ProducerPropertiesFilterDialect><tev:MessageContentSchemaLocation>http://www.onvif.org/onvif/ver10/schema/onvif.xsd</tev:MessageContentSchemaLocation>");
    return SOAP_OK;
}

int __tev__CreatePullPointSubscription(soap* ctx, char*, char*& response) {
    if (const int err = require_auth(ctx); err != SOAP_OK) return err;
    response = soap_xml(ctx, "<tev:SubscriptionReference><wsa:Address>/onvif/events_service</wsa:Address></tev:SubscriptionReference><wsnt:CurrentTime>" + now_utc() + "</wsnt:CurrentTime><wsnt:TerminationTime>" + now_utc() + "</wsnt:TerminationTime>");
    return SOAP_OK;
}

int __tev__PullMessages(soap* ctx, char*, char*& response) {
    if (const int err = require_auth(ctx); err != SOAP_OK) return err;
    response = soap_xml(ctx, "<tev:CurrentTime>" + now_utc() + "</tev:CurrentTime><tev:TerminationTime>" + now_utc() + "</tev:TerminationTime>");
    return SOAP_OK;
}

int __wsnt__Renew(soap* ctx, char*, char*& response) {
    if (const int err = require_auth(ctx); err != SOAP_OK) return err;
    response = soap_xml(ctx, "<wsnt:TerminationTime>" + now_utc() + "</wsnt:TerminationTime>");
    return SOAP_OK;
}

int __wsnt__Unsubscribe(soap* ctx, char*, char*& response) {
    if (const int err = require_auth(ctx); err != SOAP_OK) return err;
    response = soap_xml(ctx, "");
    return SOAP_OK;
}

int __tosd__GetServiceCapabilities(soap* ctx, char*, char*& response) {
    if (const int err = require_auth(ctx); err != SOAP_OK) return err;
    response = soap_xml(ctx, "<tosd:Capabilities MaximumNumberOfOSDs=\"1\" TemporaryOSDText=\"true\"/>");
    return SOAP_OK;
}

int __tosd__GetOSDs(soap* ctx, char*, char*& response) {
    if (const int err = require_auth(ctx); err != SOAP_OK) return err;
    response = soap_xml(ctx, "<tosd:OSDs xmlns:tt=\"http://www.onvif.org/ver10/schema\" token=\"osd-main\"><tt:VideoSourceConfigurationToken>video-source</tt:VideoSourceConfigurationToken><tt:Type>Text</tt:Type><tt:Position><tt:Type>UpperLeft</tt:Type></tt:Position><tt:TextString><tt:Type>Plain</tt:Type><tt:PlainText>Afterveda</tt:PlainText></tt:TextString></tosd:OSDs>");
    return SOAP_OK;
}

int __tosd__GetOSD(soap* ctx, char*, char*& response) {
    return __tosd__GetOSDs(ctx, nullptr, response);
}

int __tosd__SetOSD(soap* ctx, char*, char*& response) {
    if (const int err = require_auth(ctx); err != SOAP_OK) return err;
    response = soap_xml(ctx, "");
    return SOAP_OK;
}

int SOAP_ENV__Fault(soap*, char*, char*, char*, SOAP_ENV__Detail*, SOAP_ENV__Code*, SOAP_ENV__Reason*, char*, char*, SOAP_ENV__Detail*) {
    return SOAP_OK;
}
