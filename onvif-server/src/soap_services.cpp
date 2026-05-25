#include "soap_services.h"

#include "ptz_service.h"
#include "rail_client.h"
#include "utils.h"

#include <ctime>
#include <sstream>

namespace afterveda_onvif {
namespace {

bool auth_ok(const Config& config, const std::string& request) {
    if (config.username.empty() && config.password.empty()) {
        return true;
    }
    return contains(request, "<wsse:Username>" + config.username + "</wsse:Username>") &&
        (config.password.empty() || contains(request, config.password));
}

std::string profile_xml(const VideoProfile& profile, bool include_ptz) {
    std::ostringstream out;
    out << "<trt:Profiles token=\"" << xml_escape(profile.token) << "\" fixed=\"false\">"
        << "<tt:Name>" << xml_escape(profile.name) << "</tt:Name>"
        << "<tt:VideoSourceConfiguration token=\"video-source\">"
        << "<tt:Name>PiCam</tt:Name><tt:UseCount>1</tt:UseCount>"
        << "<tt:SourceToken>picam-source</tt:SourceToken>"
        << "<tt:Bounds x=\"0\" y=\"0\" width=\"" << profile.width << "\" height=\"" << profile.height << "\"/>"
        << "</tt:VideoSourceConfiguration>"
        << "<tt:VideoEncoderConfiguration token=\"encoder-" << xml_escape(profile.token) << "\">"
        << "<tt:Name>" << xml_escape(profile.name) << "-h264</tt:Name><tt:UseCount>1</tt:UseCount>"
        << "<tt:Encoding>H264</tt:Encoding>"
        << "<tt:Resolution><tt:Width>" << profile.width << "</tt:Width><tt:Height>" << profile.height << "</tt:Height></tt:Resolution>"
        << "<tt:Quality>4</tt:Quality>"
        << "<tt:RateControl><tt:FrameRateLimit>" << profile.fps << "</tt:FrameRateLimit>"
        << "<tt:EncodingInterval>1</tt:EncodingInterval><tt:BitrateLimit>" << profile.bitrate_kbps << "</tt:BitrateLimit></tt:RateControl>"
        << "<tt:H264><tt:GovLength>" << profile.fps << "</tt:GovLength><tt:H264Profile>Main</tt:H264Profile></tt:H264>"
        << "<tt:Multicast><tt:Address><tt:Type>IPv4</tt:Type><tt:IPv4Address>0.0.0.0</tt:IPv4Address></tt:Address>"
        << "<tt:Port>0</tt:Port><tt:TTL>1</tt:TTL><tt:AutoStart>false</tt:AutoStart></tt:Multicast>"
        << "<tt:SessionTimeout>PT60S</tt:SessionTimeout>"
        << "</tt:VideoEncoderConfiguration>";
    if (include_ptz) {
        out << "<tt:PTZConfiguration token=\"ptz\">"
            << "<tt:Name>PanTilt</tt:Name><tt:UseCount>1</tt:UseCount><tt:NodeToken>ptz-node</tt:NodeToken>"
            << "<tt:DefaultContinuousPanTiltVelocitySpace>http://www.onvif.org/ver10/tptz/PanTiltSpaces/VelocityGenericSpace</tt:DefaultContinuousPanTiltVelocitySpace>"
            << "<tt:DefaultPTZTimeout>PT1S</tt:DefaultPTZTimeout>"
            << "</tt:PTZConfiguration>";
    }
    out << "</trt:Profiles>";
    return out.str();
}

std::string get_capabilities(const Config& config) {
    const std::string base = xaddr_base(config);
    return soap_envelope(
        "<tds:GetCapabilitiesResponse><tds:Capabilities>"
        "<tt:Device><tt:XAddr>" + xml_escape(base + "/device_service") + "</tt:XAddr></tt:Device>"
        "<tt:Media><tt:XAddr>" + xml_escape(base + "/media_service") + "</tt:XAddr>"
        "<tt:StreamingCapabilities><tt:RTPMulticast>false</tt:RTPMulticast><tt:RTP_TCP>true</tt:RTP_TCP><tt:RTP_RTSP_TCP>true</tt:RTP_RTSP_TCP></tt:StreamingCapabilities>"
        "</tt:Media>"
        "<tt:PTZ><tt:XAddr>" + xml_escape(base + "/ptz_service") + "</tt:XAddr></tt:PTZ>"
        "<tt:Imaging><tt:XAddr>" + xml_escape(base + "/imaging_service") + "</tt:XAddr></tt:Imaging>"
        "<tt:Events><tt:XAddr>" + xml_escape(base + "/events_service") + "</tt:XAddr></tt:Events>"
        "</tds:Capabilities></tds:GetCapabilitiesResponse>");
}

std::string get_services(const Config& config) {
    const std::string base = xaddr_base(config);
    return soap_envelope(
        "<tds:GetServicesResponse>"
        "<tds:Service><tds:Namespace>http://www.onvif.org/ver10/device/wsdl</tds:Namespace><tds:XAddr>" + xml_escape(base + "/device_service") + "</tds:XAddr><tds:Version><tt:Major>2</tt:Major><tt:Minor>20</tt:Minor></tds:Version></tds:Service>"
        "<tds:Service><tds:Namespace>http://www.onvif.org/ver10/media/wsdl</tds:Namespace><tds:XAddr>" + xml_escape(base + "/media_service") + "</tds:XAddr><tds:Version><tt:Major>2</tt:Major><tt:Minor>20</tt:Minor></tds:Version></tds:Service>"
        "<tds:Service><tds:Namespace>http://www.onvif.org/ver20/ptz/wsdl</tds:Namespace><tds:XAddr>" + xml_escape(base + "/ptz_service") + "</tds:XAddr><tds:Version><tt:Major>2</tt:Major><tt:Minor>20</tt:Minor></tds:Version></tds:Service>"
        "</tds:GetServicesResponse>");
}

std::string get_device_information(const Config& config) {
    return soap_envelope(
        "<tds:GetDeviceInformationResponse>"
        "<tds:Manufacturer>" + xml_escape(config.manufacturer) + "</tds:Manufacturer>"
        "<tds:Model>" + xml_escape(config.model) + "</tds:Model>"
        "<tds:FirmwareVersion>" + xml_escape(config.firmware) + "</tds:FirmwareVersion>"
        "<tds:SerialNumber>" + xml_escape(config.serial) + "</tds:SerialNumber>"
        "<tds:HardwareId>" + xml_escape(config.hardware_id) + "</tds:HardwareId>"
        "</tds:GetDeviceInformationResponse>");
}

std::string get_system_date_and_time() {
    std::time_t raw = std::time(nullptr);
    std::tm tm{};
    gmtime_r(&raw, &tm);
    return soap_envelope(
        "<tds:GetSystemDateAndTimeResponse><tds:SystemDateAndTime>"
        "<tt:DateTimeType>NTP</tt:DateTimeType><tt:DaylightSavings>false</tt:DaylightSavings>"
        "<tt:UTCDateTime><tt:Time><tt:Hour>" + std::to_string(tm.tm_hour) + "</tt:Hour><tt:Minute>" + std::to_string(tm.tm_min) + "</tt:Minute><tt:Second>" + std::to_string(tm.tm_sec) + "</tt:Second></tt:Time>"
        "<tt:Date><tt:Year>" + std::to_string(tm.tm_year + 1900) + "</tt:Year><tt:Month>" + std::to_string(tm.tm_mon + 1) + "</tt:Month><tt:Day>" + std::to_string(tm.tm_mday) + "</tt:Day></tt:Date></tt:UTCDateTime>"
        "</tds:SystemDateAndTime></tds:GetSystemDateAndTimeResponse>");
}

std::string get_profiles(const Config& config) {
    std::string body = "<trt:GetProfilesResponse>";
    for (const auto& profile : profiles_from_rail(config)) {
        body += profile_xml(profile, true);
    }
    body += "</trt:GetProfilesResponse>";
    return soap_envelope(body);
}

std::string get_stream_uri(const Config& config) {
    return soap_envelope(
        "<trt:GetStreamUriResponse><trt:MediaUri>"
        "<tt:Uri>" + xml_escape(config.rtsp_uri) + "</tt:Uri>"
        "<tt:InvalidAfterConnect>false</tt:InvalidAfterConnect>"
        "<tt:InvalidAfterReboot>false</tt:InvalidAfterReboot>"
        "<tt:Timeout>PT60S</tt:Timeout>"
        "</trt:MediaUri></trt:GetStreamUriResponse>");
}

std::string get_video_encoder_configuration(const Config& config, const std::string& request) {
    const VideoProfile profile = find_profile(config, extract_profile_token(request));
    return soap_envelope(
        "<trt:GetVideoEncoderConfigurationResponse>"
        "<trt:Configuration token=\"encoder-" + xml_escape(profile.token) + "\">"
        "<tt:Name>" + xml_escape(profile.name) + "-h264</tt:Name><tt:UseCount>1</tt:UseCount><tt:Encoding>H264</tt:Encoding>"
        "<tt:Resolution><tt:Width>" + std::to_string(profile.width) + "</tt:Width><tt:Height>" + std::to_string(profile.height) + "</tt:Height></tt:Resolution>"
        "<tt:Quality>4</tt:Quality><tt:RateControl><tt:FrameRateLimit>" + std::to_string(profile.fps) + "</tt:FrameRateLimit><tt:EncodingInterval>1</tt:EncodingInterval><tt:BitrateLimit>" + std::to_string(profile.bitrate_kbps) + "</tt:BitrateLimit></tt:RateControl>"
        "<tt:H264><tt:GovLength>" + std::to_string(profile.fps) + "</tt:GovLength><tt:H264Profile>Main</tt:H264Profile></tt:H264><tt:SessionTimeout>PT60S</tt:SessionTimeout>"
        "</trt:Configuration></trt:GetVideoEncoderConfigurationResponse>");
}

std::string set_video_encoder_configuration(const Config& config, const std::string& request) {
    const std::string token = first_nonempty({
        extract_between(request, "token=\"encoder-", "\""),
        extract_between(request, "<trt:ProfileToken>", "</trt:ProfileToken>"),
        extract_between(request, "<ProfileToken>", "</ProfileToken>"),
    });
    switch_rail_profile(config, token);
    return soap_envelope("<trt:SetVideoEncoderConfigurationResponse/>");
}

std::string handle_imaging(const std::string& request) {
    if (contains(request, "GetOptions")) {
        return soap_envelope(
            "<timg:GetOptionsResponse xmlns:timg=\"http://www.onvif.org/ver20/imaging/wsdl\">"
            "<timg:ImagingOptions>"
            "<tt:Brightness><tt:Min>0</tt:Min><tt:Max>100</tt:Max></tt:Brightness>"
            "<tt:Contrast><tt:Min>0</tt:Min><tt:Max>100</tt:Max></tt:Contrast>"
            "<tt:ColorSaturation><tt:Min>0</tt:Min><tt:Max>100</tt:Max></tt:ColorSaturation>"
            "</timg:ImagingOptions></timg:GetOptionsResponse>");
    }
    if (contains(request, "GetImagingSettings")) {
        return soap_envelope(
            "<timg:GetImagingSettingsResponse xmlns:timg=\"http://www.onvif.org/ver20/imaging/wsdl\">"
            "<timg:ImagingSettings>"
            "<tt:Brightness>50</tt:Brightness><tt:Contrast>50</tt:Contrast><tt:ColorSaturation>50</tt:ColorSaturation>"
            "</timg:ImagingSettings></timg:GetImagingSettingsResponse>");
    }
    if (contains(request, "SetImagingSettings")) {
        return soap_envelope("<timg:SetImagingSettingsResponse xmlns:timg=\"http://www.onvif.org/ver20/imaging/wsdl\"/>");
    }
    return soap_fault("Unsupported imaging action");
}

std::string handle_events(const std::string& request) {
    if (contains(request, "GetEventProperties")) {
        return soap_envelope(
            "<tev:GetEventPropertiesResponse xmlns:tev=\"http://www.onvif.org/ver10/events/wsdl\">"
            "<tev:TopicNamespaceLocation>http://www.onvif.org/onvif/ver10/topics/topicns.xml</tev:TopicNamespaceLocation>"
            "<wsnt:FixedTopicSet xmlns:wsnt=\"http://docs.oasis-open.org/wsn/b-2\">true</wsnt:FixedTopicSet>"
            "<tev:TopicSet/>"
            "<tev:TopicExpressionDialect>http://www.onvif.org/ver10/tev/topicExpression/ConcreteSet</tev:TopicExpressionDialect>"
            "<tev:MessageContentFilterDialect>http://www.onvif.org/ver10/tev/messageContentFilter/ItemFilter</tev:MessageContentFilterDialect>"
            "<tev:ProducerPropertiesFilterDialect>http://www.onvif.org/ver10/tev/messageContentFilter/ItemFilter</tev:ProducerPropertiesFilterDialect>"
            "<tev:MessageContentSchemaLocation>http://www.onvif.org/onvif/ver10/schema/onvif.xsd</tev:MessageContentSchemaLocation>"
            "</tev:GetEventPropertiesResponse>");
    }
    return soap_fault("Unsupported events action");
}

}  // namespace

HttpResponse handle_soap(const Config& config, const std::string& request) {
    if (!auth_ok(config, request)) {
        return {401, "Unauthorized", "application/soap+xml; charset=utf-8", soap_fault("ONVIF authentication required")};
    }

    std::string body;
    if (contains(request, "GetCapabilities")) {
        body = get_capabilities(config);
    } else if (contains(request, "GetServices")) {
        body = get_services(config);
    } else if (contains(request, "GetDeviceInformation")) {
        body = get_device_information(config);
    } else if (contains(request, "GetSystemDateAndTime")) {
        body = get_system_date_and_time();
    } else if (contains(request, "GetProfiles")) {
        body = get_profiles(config);
    } else if (contains(request, "GetStreamUri")) {
        body = get_stream_uri(config);
    } else if (contains(request, "GetVideoEncoderConfiguration")) {
        body = get_video_encoder_configuration(config, request);
    } else if (contains(request, "SetVideoEncoderConfiguration")) {
        body = set_video_encoder_configuration(config, request);
    } else if (contains(request, "ContinuousMove") || contains(request, "RelativeMove") ||
               contains(request, "Stop") || contains(request, "GetStatus") ||
               contains(request, "GotoHomePosition") || contains(request, "SetHomePosition")) {
        body = handle_ptz(config, request);
    } else if (contains(request, "GetImagingSettings") || contains(request, "SetImagingSettings") ||
               contains(request, "GetOptions")) {
        body = handle_imaging(request);
    } else if (contains(request, "GetEventProperties")) {
        body = handle_events(request);
    } else {
        body = soap_fault("Unsupported ONVIF action");
    }

    const bool fault = contains(body, "<s:Fault>");
    return {fault ? 500 : 200, fault ? "Internal Server Error" : "OK", "application/soap+xml; charset=utf-8", body};
}

}  // namespace afterveda_onvif
