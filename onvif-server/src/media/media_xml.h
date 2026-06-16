#pragma once

#include "config.h"
#include "media_model.h"

#include <string>

namespace afterveda_onvif {

std::string device_capabilities_xml(const Config& config);
std::string media1_service_capabilities_xml();
std::string media2_service_capabilities_xml();
std::string media1_profile_xml(const VideoProfile& profile);
std::string media1_get_profile_xml(const VideoProfile& profile);
std::string media1_profiles_xml(const MediaModel& model);
std::string media2_profiles_xml(const MediaModel& model);
std::string media1_stream_uri_xml(const MediaModel& model);
std::string media2_stream_uri_xml(const MediaModel& model);
std::string media1_video_sources_xml(const MediaModel& model);
std::string media1_video_source_configurations_xml(const MediaModel& model);
std::string media1_video_source_configuration_xml(const MediaModel& model);
std::string media1_video_encoder_configurations_xml(const MediaModel& model);
std::string media1_video_encoder_configuration_xml(const VideoProfile& profile);
std::string media1_video_source_configuration_options_xml(const MediaModel& model);
std::string media1_video_encoder_configuration_options_xml(const MediaModel& model);

}  // namespace afterveda_onvif
