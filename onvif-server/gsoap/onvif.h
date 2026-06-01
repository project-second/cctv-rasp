#import "soap12.h"
#import "wsse.h"
#import "wsdd10.h"
#import "wsa.h"

typedef char* XML;

//gsoap tt schema namespace: http://www.onvif.org/ver10/schema
//gsoap wstop schema namespace: http://docs.oasis-open.org/wsn/t-1
//gsoap tns1 schema namespace: http://www.onvif.org/ver10/topics

//gsoap tds service name: DeviceBinding
//gsoap tds service namespace: http://www.onvif.org/ver10/device/wsdl
//gsoap tds service style: document
//gsoap tds service encoding: literal
//gsoap tds service method-action: GetServices http://www.onvif.org/ver10/device/wsdl/GetServices
int __tds__GetServices(XML tds__GetServices, XML& tds__GetServicesResponse);
//gsoap tds service method-action: GetServiceCapabilities http://www.onvif.org/ver10/device/wsdl/GetServiceCapabilities
int __tds__GetServiceCapabilities(XML tds__GetServiceCapabilities, XML& tds__GetServiceCapabilitiesResponse);
//gsoap tds service method-action: GetDeviceInformation http://www.onvif.org/ver10/device/wsdl/GetDeviceInformation
int __tds__GetDeviceInformation(XML tds__GetDeviceInformation, XML& tds__GetDeviceInformationResponse);
//gsoap tds service method-action: GetSystemDateAndTime http://www.onvif.org/ver10/device/wsdl/GetSystemDateAndTime
int __tds__GetSystemDateAndTime(XML tds__GetSystemDateAndTime, XML& tds__GetSystemDateAndTimeResponse);
//gsoap tds service method-action: GetScopes http://www.onvif.org/ver10/device/wsdl/GetScopes
int __tds__GetScopes(XML tds__GetScopes, XML& tds__GetScopesResponse);
//gsoap tds service method-action: GetHostname http://www.onvif.org/ver10/device/wsdl/GetHostname
int __tds__GetHostname(XML tds__GetHostname, XML& tds__GetHostnameResponse);
//gsoap tds service method-action: GetNetworkInterfaces http://www.onvif.org/ver10/device/wsdl/GetNetworkInterfaces
int __tds__GetNetworkInterfaces(XML tds__GetNetworkInterfaces, XML& tds__GetNetworkInterfacesResponse);
//gsoap tds service method-action: GetUsers http://www.onvif.org/ver10/device/wsdl/GetUsers
int __tds__GetUsers(XML tds__GetUsers, XML& tds__GetUsersResponse);
//gsoap tds service method-action: GetCapabilities http://www.onvif.org/ver10/device/wsdl/GetCapabilities
int __tds__GetCapabilities(XML tds__GetCapabilities, XML& tds__GetCapabilitiesResponse);

//gsoap trt service name: MediaBinding
//gsoap trt service namespace: http://www.onvif.org/ver10/media/wsdl
//gsoap trt service style: document
//gsoap trt service encoding: literal
//gsoap trt service method-action: GetProfiles http://www.onvif.org/ver10/media/wsdl/GetProfiles
int __trt__GetProfiles(XML trt__GetProfiles, XML& trt__GetProfilesResponse);
//gsoap trt service method-action: GetServiceCapabilities http://www.onvif.org/ver10/media/wsdl/GetServiceCapabilities
int __trt__GetServiceCapabilities(XML trt__GetServiceCapabilities, XML& trt__GetServiceCapabilitiesResponse);
//gsoap trt service method-action: GetVideoSources http://www.onvif.org/ver10/media/wsdl/GetVideoSources
int __trt__GetVideoSources(XML trt__GetVideoSources, XML& trt__GetVideoSourcesResponse);
//gsoap trt service method-action: GetVideoSourceConfigurations http://www.onvif.org/ver10/media/wsdl/GetVideoSourceConfigurations
int __trt__GetVideoSourceConfigurations(XML trt__GetVideoSourceConfigurations, XML& trt__GetVideoSourceConfigurationsResponse);
//gsoap trt service method-action: GetVideoSourceConfiguration http://www.onvif.org/ver10/media/wsdl/GetVideoSourceConfiguration
int __trt__GetVideoSourceConfiguration(XML trt__GetVideoSourceConfiguration, XML& trt__GetVideoSourceConfigurationResponse);
//gsoap trt service method-action: GetVideoEncoderConfigurations http://www.onvif.org/ver10/media/wsdl/GetVideoEncoderConfigurations
int __trt__GetVideoEncoderConfigurations(XML trt__GetVideoEncoderConfigurations, XML& trt__GetVideoEncoderConfigurationsResponse);
//gsoap trt service method-action: GetStreamUri http://www.onvif.org/ver10/media/wsdl/GetStreamUri
int __trt__GetStreamUri(XML trt__GetStreamUri, XML& trt__GetStreamUriResponse);
//gsoap trt service method-action: GetVideoEncoderConfiguration http://www.onvif.org/ver10/media/wsdl/GetVideoEncoderConfiguration
int __trt__GetVideoEncoderConfiguration(XML trt__GetVideoEncoderConfiguration, XML& trt__GetVideoEncoderConfigurationResponse);
//gsoap trt service method-action: GetVideoSourceConfigurationOptions http://www.onvif.org/ver10/media/wsdl/GetVideoSourceConfigurationOptions
int __trt__GetVideoSourceConfigurationOptions(XML trt__GetVideoSourceConfigurationOptions, XML& trt__GetVideoSourceConfigurationOptionsResponse);
//gsoap trt service method-action: GetVideoEncoderConfigurationOptions http://www.onvif.org/ver10/media/wsdl/GetVideoEncoderConfigurationOptions
int __trt__GetVideoEncoderConfigurationOptions(XML trt__GetVideoEncoderConfigurationOptions, XML& trt__GetVideoEncoderConfigurationOptionsResponse);
//gsoap trt service method-action: SetVideoEncoderConfiguration http://www.onvif.org/ver10/media/wsdl/SetVideoEncoderConfiguration
int __trt__SetVideoEncoderConfiguration(XML trt__SetVideoEncoderConfiguration, XML& trt__SetVideoEncoderConfigurationResponse);

//gsoap tr2 service name: Media2Binding
//gsoap tr2 service namespace: http://www.onvif.org/ver20/media/wsdl
//gsoap tr2 service style: document
//gsoap tr2 service encoding: literal
//gsoap tr2 service method-action: GetProfiles http://www.onvif.org/ver20/media/wsdl/GetProfiles
int __tr2__GetProfiles(XML tr2__GetProfiles, XML& tr2__GetProfilesResponse);
//gsoap tr2 service method-action: GetStreamUri http://www.onvif.org/ver20/media/wsdl/GetStreamUri
int __tr2__GetStreamUri(XML tr2__GetStreamUri, XML& tr2__GetStreamUriResponse);
//gsoap tr2 service method-action: GetServiceCapabilities http://www.onvif.org/ver20/media/wsdl/GetServiceCapabilities
int __tr2__GetServiceCapabilities(XML tr2__GetServiceCapabilities, XML& tr2__GetServiceCapabilitiesResponse);

//gsoap tptz service name: PTZBinding
//gsoap tptz service namespace: http://www.onvif.org/ver20/ptz/wsdl
//gsoap tptz service style: document
//gsoap tptz service encoding: literal
//gsoap tptz service method-action: GetNodes http://www.onvif.org/ver20/ptz/wsdl/GetNodes
int __tptz__GetNodes(XML tptz__GetNodes, XML& tptz__GetNodesResponse);
//gsoap tptz service method-action: GetConfigurations http://www.onvif.org/ver20/ptz/wsdl/GetConfigurations
int __tptz__GetConfigurations(XML tptz__GetConfigurations, XML& tptz__GetConfigurationsResponse);
//gsoap tptz service method-action: GetPresets http://www.onvif.org/ver20/ptz/wsdl/GetPresets
int __tptz__GetPresets(XML tptz__GetPresets, XML& tptz__GetPresetsResponse);
//gsoap tptz service method-action: SetPreset http://www.onvif.org/ver20/ptz/wsdl/SetPreset
int __tptz__SetPreset(XML tptz__SetPreset, XML& tptz__SetPresetResponse);
//gsoap tptz service method-action: GotoPreset http://www.onvif.org/ver20/ptz/wsdl/GotoPreset
int __tptz__GotoPreset(XML tptz__GotoPreset, XML& tptz__GotoPresetResponse);
//gsoap tptz service method-action: GetStatus http://www.onvif.org/ver20/ptz/wsdl/GetStatus
int __tptz__GetStatus(XML tptz__GetStatus, XML& tptz__GetStatusResponse);
//gsoap tptz service method-action: GotoHomePosition http://www.onvif.org/ver20/ptz/wsdl/GotoHomePosition
int __tptz__GotoHomePosition(XML tptz__GotoHomePosition, XML& tptz__GotoHomePositionResponse);
//gsoap tptz service method-action: SetHomePosition http://www.onvif.org/ver20/ptz/wsdl/SetHomePosition
int __tptz__SetHomePosition(XML tptz__SetHomePosition, XML& tptz__SetHomePositionResponse);
//gsoap tptz service method-action: ContinuousMove http://www.onvif.org/ver20/ptz/wsdl/ContinuousMove
int __tptz__ContinuousMove(XML tptz__ContinuousMove, XML& tptz__ContinuousMoveResponse);
//gsoap tptz service method-action: RelativeMove http://www.onvif.org/ver20/ptz/wsdl/RelativeMove
int __tptz__RelativeMove(XML tptz__RelativeMove, XML& tptz__RelativeMoveResponse);
//gsoap tptz service method-action: Stop http://www.onvif.org/ver20/ptz/wsdl/Stop
int __tptz__Stop(XML tptz__Stop, XML& tptz__StopResponse);

//gsoap timg service name: ImagingBinding
//gsoap timg service namespace: http://www.onvif.org/ver20/imaging/wsdl
//gsoap timg service style: document
//gsoap timg service encoding: literal
//gsoap timg service method-action: GetServiceCapabilities http://www.onvif.org/ver20/imaging/wsdl/GetServiceCapabilities
int __timg__GetServiceCapabilities(XML timg__GetServiceCapabilities, XML& timg__GetServiceCapabilitiesResponse);
//gsoap timg service method-action: GetImagingSettings http://www.onvif.org/ver20/imaging/wsdl/GetImagingSettings
int __timg__GetImagingSettings(XML timg__GetImagingSettings, XML& timg__GetImagingSettingsResponse);
//gsoap timg service method-action: SetImagingSettings http://www.onvif.org/ver20/imaging/wsdl/SetImagingSettings
int __timg__SetImagingSettings(XML timg__SetImagingSettings, XML& timg__SetImagingSettingsResponse);
//gsoap timg service method-action: GetOptions http://www.onvif.org/ver20/imaging/wsdl/GetOptions
int __timg__GetOptions(XML timg__GetOptions, XML& timg__GetOptionsResponse);

//gsoap tev service name: EventsBinding
//gsoap tev service namespace: http://www.onvif.org/ver10/events/wsdl
//gsoap tev service style: document
//gsoap tev service encoding: literal
//gsoap tev service method-action: GetEventProperties http://www.onvif.org/ver10/events/wsdl/GetEventProperties
int __tev__GetEventProperties(XML tev__GetEventProperties, XML& tev__GetEventPropertiesResponse);
//gsoap tev service method-action: CreatePullPointSubscription http://www.onvif.org/ver10/events/wsdl/CreatePullPointSubscription
int __tev__CreatePullPointSubscription(XML tev__CreatePullPointSubscription, XML& tev__CreatePullPointSubscriptionResponse);
//gsoap tev service method-action: PullMessages http://www.onvif.org/ver10/events/wsdl/PullMessages
int __tev__PullMessages(XML tev__PullMessages, XML& tev__PullMessagesResponse);

//gsoap wsnt service name: NotificationBinding
//gsoap wsnt service namespace: http://docs.oasis-open.org/wsn/b-2
//gsoap wsnt service style: document
//gsoap wsnt service encoding: literal
//gsoap wsnt service method-action: Renew http://docs.oasis-open.org/wsn/bw-2/SubscriptionManager/RenewRequest
int __wsnt__Renew(XML wsnt__Renew, XML& wsnt__RenewResponse);
//gsoap wsnt service method-action: Unsubscribe http://docs.oasis-open.org/wsn/bw-2/SubscriptionManager/UnsubscribeRequest
int __wsnt__Unsubscribe(XML wsnt__Unsubscribe, XML& wsnt__UnsubscribeResponse);

//gsoap tosd service name: OSDConfigurationBinding
//gsoap tosd service namespace: http://www.onvif.org/ver10/osd/wsdl
//gsoap tosd service style: document
//gsoap tosd service encoding: literal
//gsoap tosd service method-action: GetServiceCapabilities http://www.onvif.org/ver10/osd/wsdl/GetServiceCapabilities
int __tosd__GetServiceCapabilities(XML tosd__GetServiceCapabilities, XML& tosd__GetServiceCapabilitiesResponse);
//gsoap tosd service method-action: GetOSDs http://www.onvif.org/ver10/osd/wsdl/GetOSDs
int __tosd__GetOSDs(XML tosd__GetOSDs, XML& tosd__GetOSDsResponse);
//gsoap tosd service method-action: GetOSD http://www.onvif.org/ver10/osd/wsdl/GetOSD
int __tosd__GetOSD(XML tosd__GetOSD, XML& tosd__GetOSDResponse);
//gsoap tosd service method-action: SetOSD http://www.onvif.org/ver10/osd/wsdl/SetOSD
int __tosd__SetOSD(XML tosd__SetOSD, XML& tosd__SetOSDResponse);
