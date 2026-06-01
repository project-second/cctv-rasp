#include "media_xml.h"

#include "utils.h"

#include <algorithm>
#include <sstream>

namespace afterveda_onvif {
namespace {

std::string int_range_xml(const std::string& tag, int min, int max) {
    return "<tt:" + tag + "><tt:Min>" + std::to_string(min) + "</tt:Min><tt:Max>" + std::to_string(max) + "</tt:Max></tt:" + tag + ">";
}

std::string bounds_xml(const VideoProfile& profile) {
    return "<tt:Bounds x=\"0\" y=\"0\" width=\"" + std::to_string(profile.width) + "\" height=\"" + std::to_string(profile.height) + "\"/>";
}

std::string multicast_xml() {
    return "<tt:Multicast>"
        "<tt:Address><tt:Type>IPv4</tt:Type><tt:IPv4Address>0.0.0.0</tt:IPv4Address></tt:Address>"
        "<tt:Port>0</tt:Port><tt:TTL>1</tt:TTL><tt:AutoStart>false</tt:AutoStart>"
        "</tt:Multicast>";
}

std::string video_source_configuration_xml(const VideoProfile& profile) {
    return "<tt:VideoSourceConfiguration token=\"" + std::string(kVideoSourceConfigToken) + "\">"
        "<tt:Name>PiCam</tt:Name>"
        "<tt:UseCount>1</tt:UseCount>"
        "<tt:SourceToken>" + std::string(kVideoSourceToken) + "</tt:SourceToken>" +
        bounds_xml(profile) +
        "</tt:VideoSourceConfiguration>";
}

std::string video_source_configuration_response_xml(const VideoProfile& profile) {
    return "<trt:Configuration xmlns:tt=\"http://www.onvif.org/ver10/schema\" token=\"" +
        std::string(kVideoSourceConfigToken) + "\">"
        "<tt:Name>PiCam</tt:Name>"
        "<tt:UseCount>1</tt:UseCount>"
        "<tt:SourceToken>" + std::string(kVideoSourceToken) + "</tt:SourceToken>" +
        bounds_xml(profile) +
        "</trt:Configuration>";
}

std::string video_encoder_configuration_xml(const VideoProfile& profile, const std::string& qname) {
    return "<" + qname + " token=\"" + xml_escape(encoder_token_for_profile(profile)) + "\">"
        "<tt:Name>" + xml_escape(profile.name) + "-h264</tt:Name>"
        "<tt:UseCount>1</tt:UseCount>"
        "<tt:Encoding>H264</tt:Encoding>"
        "<tt:Resolution><tt:Width>" + std::to_string(profile.width) + "</tt:Width><tt:Height>" + std::to_string(profile.height) + "</tt:Height></tt:Resolution>"
        "<tt:Quality>4</tt:Quality>"
        "<tt:RateControl><tt:FrameRateLimit>" + std::to_string(profile.fps) + "</tt:FrameRateLimit>"
        "<tt:EncodingInterval>1</tt:EncodingInterval><tt:BitrateLimit>" + std::to_string(profile.bitrate_kbps) + "</tt:BitrateLimit></tt:RateControl>"
        "<tt:H264><tt:GovLength>" + std::to_string(profile.fps * 2) + "</tt:GovLength><tt:H264Profile>Main</tt:H264Profile></tt:H264>" +
        multicast_xml() +
        "<tt:SessionTimeout>" + std::string(kMediaSessionTimeout) + "</tt:SessionTimeout>"
        "</" + qname + ">";
}

std::string ptz_configuration_xml() {
    return "<tt:PTZConfiguration token=\"" + std::string(kPtzConfigToken) + "\">"
        "<tt:Name>PanTilt</tt:Name><tt:UseCount>1</tt:UseCount><tt:NodeToken>ptz-node</tt:NodeToken>"
        "<tt:DefaultContinuousPanTiltVelocitySpace>http://www.onvif.org/ver10/tptz/PanTiltSpaces/VelocityGenericSpace</tt:DefaultContinuousPanTiltVelocitySpace>"
        "<tt:DefaultRelativePanTiltTranslationSpace>http://www.onvif.org/ver10/tptz/PanTiltSpaces/TranslationGenericSpace</tt:DefaultRelativePanTiltTranslationSpace>"
        "<tt:DefaultPTZTimeout>PT1S</tt:DefaultPTZTimeout>"
        "</tt:PTZConfiguration>";
}

std::string media1_profile_xml(const VideoProfile& profile) {
    return "<trt:Profiles token=\"" + xml_escape(profile.token) + "\" fixed=\"false\">"
        "<tt:Name>" + xml_escape(profile.name) + "</tt:Name>" +
        video_source_configuration_xml(profile) +
        video_encoder_configuration_xml(profile, "tt:VideoEncoderConfiguration") +
        ptz_configuration_xml() +
        "</trt:Profiles>";
}

std::string media2_profile_xml(const VideoProfile& profile) {
    return "<tr2:Profiles token=\"" + xml_escape(profile.token) + "\" fixed=\"false\">"
        "<tt:Name>" + xml_escape(profile.name) + "</tt:Name>"
        "<tr2:Configurations>"
        "<tt:VideoSource token=\"" + std::string(kVideoSourceConfigToken) + "\">"
        "<tt:Name>PiCam</tt:Name><tt:UseCount>1</tt:UseCount><tt:SourceToken>" + std::string(kVideoSourceToken) + "</tt:SourceToken>" +
        bounds_xml(profile) +
        "</tt:VideoSource>"
        "<tt:VideoEncoder token=\"" + xml_escape(encoder_token_for_profile(profile)) + "\">"
        "<tt:Name>" + xml_escape(profile.name) + "-h264</tt:Name><tt:UseCount>1</tt:UseCount>"
        "<tt:Encoding>H264</tt:Encoding>"
        "<tt:Resolution><tt:Width>" + std::to_string(profile.width) + "</tt:Width><tt:Height>" + std::to_string(profile.height) + "</tt:Height></tt:Resolution>"
        "<tt:RateControl><tt:FrameRateLimit>" + std::to_string(profile.fps) + "</tt:FrameRateLimit><tt:BitrateLimit>" + std::to_string(profile.bitrate_kbps) + "</tt:BitrateLimit></tt:RateControl>"
        "</tt:VideoEncoder>"
        "</tr2:Configurations>"
        "</tr2:Profiles>";
}

VideoProfile first_profile(const MediaModel& model) {
    return profile_for_token(model, "main");
}

}  // namespace

std::string device_capabilities_xml(const Config& config) {
    const std::string base = xaddr_base(config);
    return "<tds:Capabilities xmlns:tt=\"http://www.onvif.org/ver10/schema\">"
        "<tt:Device><tt:XAddr>" + xml_escape(base + "/device_service") + "</tt:XAddr></tt:Device>"
        "<tt:Media><tt:XAddr>" + xml_escape(base + "/media_service") + "</tt:XAddr>"
        "<tt:StreamingCapabilities><tt:RTPMulticast>false</tt:RTPMulticast><tt:RTP_TCP>true</tt:RTP_TCP><tt:RTP_RTSP_TCP>true</tt:RTP_RTSP_TCP></tt:StreamingCapabilities>"
        "</tt:Media>"
        "<tt:Media2><tt:XAddr>" + xml_escape(base + "/media2_service") + "</tt:XAddr>"
        "<tt:StreamingCapabilities><tt:RTPMulticast>false</tt:RTPMulticast><tt:RTP_RTSP_TCP>true</tt:RTP_RTSP_TCP></tt:StreamingCapabilities>"
        "</tt:Media2>"
        "<tt:PTZ><tt:XAddr>" + xml_escape(base + "/ptz_service") + "</tt:XAddr></tt:PTZ>"
        "<tt:Imaging><tt:XAddr>" + xml_escape(base + "/imaging_service") + "</tt:XAddr></tt:Imaging>"
        "<tt:Events><tt:XAddr>" + xml_escape(base + "/events_service") + "</tt:XAddr></tt:Events>"
        "<tt:Extension><tt:OSD><tt:XAddr>" + xml_escape(base + "/osd_service") + "</tt:XAddr></tt:OSD></tt:Extension>"
        "</tds:Capabilities>";
}

std::string media1_service_capabilities_xml() {
    return "<trt:Capabilities SnapshotUri=\"false\" Rotation=\"false\" VideoSourceMode=\"false\" "
        "OSD=\"true\" TemporaryOSDText=\"true\" EXICompression=\"false\"/>";
}

std::string media2_service_capabilities_xml() {
    return "<tr2:Capabilities SnapshotUri=\"false\" Rotation=\"false\" VideoSourceMode=\"false\" "
        "OSD=\"true\" TemporaryOSDText=\"true\" EXICompression=\"false\"/>";
}

std::string media1_profiles_xml(const MediaModel& model) {
    std::string body;
    for (const auto& profile : model.profiles) {
        body += media1_profile_xml(profile);
    }
    return body;
}

std::string media2_profiles_xml(const MediaModel& model) {
    std::string body;
    for (const auto& profile : model.profiles) {
        body += media2_profile_xml(profile);
    }
    return body;
}

std::string media1_stream_uri_xml(const MediaModel& model) {
    return "<trt:MediaUri xmlns:tt=\"http://www.onvif.org/ver10/schema\">"
        "<tt:Uri>" + xml_escape(model.rtsp_uri) + "</tt:Uri>"
        "<tt:InvalidAfterConnect>false</tt:InvalidAfterConnect>"
        "<tt:InvalidAfterReboot>false</tt:InvalidAfterReboot>"
        "<tt:Timeout>" + std::string(kMediaSessionTimeout) + "</tt:Timeout>"
        "</trt:MediaUri>";
}

std::string media2_stream_uri_xml(const MediaModel& model) {
    return "<tr2:Uri>" + xml_escape(model.rtsp_uri) + "</tr2:Uri>";
}

std::string media1_video_sources_xml(const MediaModel& model) {
    const VideoProfile profile = first_profile(model);
    return "<trt:VideoSources token=\"" + std::string(kVideoSourceToken) + "\">"
        "<tt:Framerate>" + std::to_string(profile.fps) + "</tt:Framerate>"
        "<tt:Resolution><tt:Width>" + std::to_string(profile.width) + "</tt:Width><tt:Height>" + std::to_string(profile.height) + "</tt:Height></tt:Resolution>"
        "</trt:VideoSources>";
}

std::string media1_video_source_configurations_xml(const MediaModel& model) {
    return "<trt:Configurations xmlns:tt=\"http://www.onvif.org/ver10/schema\" token=\"" +
        std::string(kVideoSourceConfigToken) + "\">"
        "<tt:Name>PiCam</tt:Name><tt:UseCount>1</tt:UseCount>"
        "<tt:SourceToken>" + std::string(kVideoSourceToken) + "</tt:SourceToken>" +
        bounds_xml(first_profile(model)) +
        "</trt:Configurations>";
}

std::string media1_video_source_configuration_xml(const MediaModel& model) {
    return video_source_configuration_response_xml(first_profile(model));
}

std::string media1_video_encoder_configurations_xml(const MediaModel& model) {
    std::string body;
    for (const auto& profile : model.profiles) {
        body += video_encoder_configuration_xml(profile, "trt:Configurations");
    }
    return body;
}

std::string media1_video_encoder_configuration_xml(const VideoProfile& profile) {
    return video_encoder_configuration_xml(profile, "trt:Configuration");
}

std::string media1_video_source_configuration_options_xml(const MediaModel& model) {
    const VideoProfile profile = first_profile(model);
    return "<trt:Options xmlns:tt=\"http://www.onvif.org/ver10/schema\">"
        "<tt:BoundsRange><tt:XRange><tt:Min>0</tt:Min><tt:Max>0</tt:Max></tt:XRange>"
        "<tt:YRange><tt:Min>0</tt:Min><tt:Max>0</tt:Max></tt:YRange>"
        "<tt:WidthRange><tt:Min>" + std::to_string(profile.width) + "</tt:Min><tt:Max>" + std::to_string(profile.width) + "</tt:Max></tt:WidthRange>"
        "<tt:HeightRange><tt:Min>" + std::to_string(profile.height) + "</tt:Min><tt:Max>" + std::to_string(profile.height) + "</tt:Max></tt:HeightRange></tt:BoundsRange>"
        "<tt:VideoSourceTokensAvailable>" + std::string(kVideoSourceToken) + "</tt:VideoSourceTokensAvailable>"
        "</trt:Options>";
}

std::string media1_video_encoder_configuration_options_xml(const MediaModel& model) {
    int min_width = 1920;
    int min_height = 1080;
    int max_width = 0;
    int max_height = 0;
    int max_fps = 0;
    int min_bitrate = 1000000;
    int max_bitrate = 0;
    for (const auto& profile : model.profiles) {
        min_width = std::min(min_width, profile.width);
        min_height = std::min(min_height, profile.height);
        max_width = std::max(max_width, profile.width);
        max_height = std::max(max_height, profile.height);
        max_fps = std::max(max_fps, profile.fps);
        min_bitrate = std::min(min_bitrate, profile.bitrate_kbps);
        max_bitrate = std::max(max_bitrate, profile.bitrate_kbps);
    }

    return "<trt:Options xmlns:tt=\"http://www.onvif.org/ver10/schema\">"
        "<tt:QualityRange><tt:Min>1</tt:Min><tt:Max>6</tt:Max></tt:QualityRange>"
        "<tt:H264>"
        "<tt:ResolutionsAvailable><tt:Width>" + std::to_string(max_width) + "</tt:Width><tt:Height>" + std::to_string(max_height) + "</tt:Height></tt:ResolutionsAvailable>"
        "<tt:GovLengthRange><tt:Min>1</tt:Min><tt:Max>" + std::to_string(max_fps * 2) + "</tt:Max></tt:GovLengthRange>"
        "<tt:FrameRateRange><tt:Min>1</tt:Min><tt:Max>" + std::to_string(max_fps) + "</tt:Max></tt:FrameRateRange>"
        "<tt:EncodingIntervalRange><tt:Min>1</tt:Min><tt:Max>1</tt:Max></tt:EncodingIntervalRange>"
        "<tt:H264ProfilesSupported>Main</tt:H264ProfilesSupported>"
        "</tt:H264>"
        "<tt:Extension>"
        "<tt:H264><tt:ResolutionsAvailable><tt:Width>" + std::to_string(min_width) + "</tt:Width><tt:Height>" + std::to_string(min_height) + "</tt:Height></tt:ResolutionsAvailable></tt:H264>"
        "</tt:Extension>" +
        int_range_xml("BitrateRange", min_bitrate, max_bitrate) +
        "</trt:Options>";
}

}  // namespace afterveda_onvif
