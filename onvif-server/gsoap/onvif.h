#import "soap12.h"
#import "wsse.h"
#import "wsdd10.h"
#import "wsa.h"

typedef char* XML;

//gsoap tt schema namespace: http://www.onvif.org/ver10/schema
//gsoap wstop schema namespace: http://docs.oasis-open.org/wsn/t-1
//gsoap tns1 schema namespace: http://www.onvif.org/ver10/topics

struct afterveda__EmptyRequest {
    XML __any;
};

struct afterveda__AnyResponse {
    XML __any;
};

struct _tds__GetDeviceInformation {
    XML __any;
};

struct _tds__GetDeviceInformationResponse {
    char* Manufacturer;
    char* Model;
    char* FirmwareVersion;
    char* SerialNumber;
    char* HardwareId;
};

struct _tptz__GetStatus {
    char* ProfileToken;
};

struct tt__Vector2D {
    double x;
    double y;
};

struct tt__Vector1D {
    double x;
};

struct tt__PTZVector {
    struct tt__Vector2D* PanTilt;
    struct tt__Vector1D* Zoom;
};

struct tt__PTZMoveStatus {
    char* PanTilt;
    char* Zoom;
};

struct tt__PTZStatus {
    struct tt__PTZVector* Position;
    struct tt__PTZMoveStatus* MoveStatus;
    char* UtcTime;
};

struct _tptz__GetStatusResponse {
    struct tt__PTZStatus* PTZStatus;
};

//gsoap tds service name: DeviceBinding
//gsoap tds service namespace: http://www.onvif.org/ver10/device/wsdl
//gsoap tds service style: document
//gsoap tds service encoding: literal
//gsoap tds service method-action: GetServices http://www.onvif.org/ver10/device/wsdl/GetServices
int __tds__GetServices(struct afterveda__EmptyRequest* tds__GetServices, struct afterveda__AnyResponse& tds__GetServicesResponse);
//gsoap tds service method-action: GetServiceCapabilities http://www.onvif.org/ver10/device/wsdl/GetServiceCapabilities
int __tds__GetServiceCapabilities(struct afterveda__EmptyRequest* tds__GetServiceCapabilities, struct afterveda__AnyResponse& tds__GetServiceCapabilitiesResponse);
//gsoap tds service method-action: GetDeviceInformation http://www.onvif.org/ver10/device/wsdl/GetDeviceInformation
int __tds__GetDeviceInformation(struct _tds__GetDeviceInformation* tds__GetDeviceInformation, struct _tds__GetDeviceInformationResponse& tds__GetDeviceInformationResponse);
//gsoap tds service method-action: GetSystemDateAndTime http://www.onvif.org/ver10/device/wsdl/GetSystemDateAndTime
int __tds__GetSystemDateAndTime(struct afterveda__EmptyRequest* tds__GetSystemDateAndTime, struct afterveda__AnyResponse& tds__GetSystemDateAndTimeResponse);
//gsoap tds service method-action: GetScopes http://www.onvif.org/ver10/device/wsdl/GetScopes
int __tds__GetScopes(struct afterveda__EmptyRequest* tds__GetScopes, struct afterveda__AnyResponse& tds__GetScopesResponse);
//gsoap tds service method-action: GetHostname http://www.onvif.org/ver10/device/wsdl/GetHostname
int __tds__GetHostname(struct afterveda__EmptyRequest* tds__GetHostname, struct afterveda__AnyResponse& tds__GetHostnameResponse);
//gsoap tds service method-action: GetNetworkInterfaces http://www.onvif.org/ver10/device/wsdl/GetNetworkInterfaces
int __tds__GetNetworkInterfaces(struct afterveda__EmptyRequest* tds__GetNetworkInterfaces, struct afterveda__AnyResponse& tds__GetNetworkInterfacesResponse);
//gsoap tds service method-action: GetUsers http://www.onvif.org/ver10/device/wsdl/GetUsers
int __tds__GetUsers(struct afterveda__EmptyRequest* tds__GetUsers, struct afterveda__AnyResponse& tds__GetUsersResponse);
//gsoap tds service method-action: GetCapabilities http://www.onvif.org/ver10/device/wsdl/GetCapabilities
int __tds__GetCapabilities(struct afterveda__EmptyRequest* tds__GetCapabilities, struct afterveda__AnyResponse& tds__GetCapabilitiesResponse);

//gsoap trt service name: MediaBinding
//gsoap trt service namespace: http://www.onvif.org/ver10/media/wsdl
//gsoap trt service style: document
//gsoap trt service encoding: literal
//gsoap trt service method-action: GetProfiles http://www.onvif.org/ver10/media/wsdl/GetProfiles
int __trt__GetProfiles(struct afterveda__EmptyRequest* trt__GetProfiles, struct afterveda__AnyResponse& trt__GetProfilesResponse);
//gsoap trt service method-action: GetProfile http://www.onvif.org/ver10/media/wsdl/GetProfile
int __trt__GetProfile(struct afterveda__EmptyRequest* trt__GetProfile, struct afterveda__AnyResponse& trt__GetProfileResponse);
//gsoap trt service method-action: GetServiceCapabilities http://www.onvif.org/ver10/media/wsdl/GetServiceCapabilities
int __trt__GetServiceCapabilities(struct afterveda__EmptyRequest* trt__GetServiceCapabilities, struct afterveda__AnyResponse& trt__GetServiceCapabilitiesResponse);
//gsoap trt service method-action: GetVideoSources http://www.onvif.org/ver10/media/wsdl/GetVideoSources
int __trt__GetVideoSources(struct afterveda__EmptyRequest* trt__GetVideoSources, struct afterveda__AnyResponse& trt__GetVideoSourcesResponse);
//gsoap trt service method-action: GetVideoSourceConfigurations http://www.onvif.org/ver10/media/wsdl/GetVideoSourceConfigurations
int __trt__GetVideoSourceConfigurations(struct afterveda__EmptyRequest* trt__GetVideoSourceConfigurations, struct afterveda__AnyResponse& trt__GetVideoSourceConfigurationsResponse);
//gsoap trt service method-action: GetVideoSourceConfiguration http://www.onvif.org/ver10/media/wsdl/GetVideoSourceConfiguration
int __trt__GetVideoSourceConfiguration(struct afterveda__EmptyRequest* trt__GetVideoSourceConfiguration, struct afterveda__AnyResponse& trt__GetVideoSourceConfigurationResponse);
//gsoap trt service method-action: GetVideoEncoderConfigurations http://www.onvif.org/ver10/media/wsdl/GetVideoEncoderConfigurations
int __trt__GetVideoEncoderConfigurations(struct afterveda__EmptyRequest* trt__GetVideoEncoderConfigurations, struct afterveda__AnyResponse& trt__GetVideoEncoderConfigurationsResponse);
//gsoap trt service method-action: GetStreamUri http://www.onvif.org/ver10/media/wsdl/GetStreamUri
int __trt__GetStreamUri(struct afterveda__EmptyRequest* trt__GetStreamUri, struct afterveda__AnyResponse& trt__GetStreamUriResponse);
//gsoap trt service method-action: GetVideoEncoderConfiguration http://www.onvif.org/ver10/media/wsdl/GetVideoEncoderConfiguration
int __trt__GetVideoEncoderConfiguration(struct afterveda__EmptyRequest* trt__GetVideoEncoderConfiguration, struct afterveda__AnyResponse& trt__GetVideoEncoderConfigurationResponse);
//gsoap trt service method-action: GetVideoSourceConfigurationOptions http://www.onvif.org/ver10/media/wsdl/GetVideoSourceConfigurationOptions
int __trt__GetVideoSourceConfigurationOptions(struct afterveda__EmptyRequest* trt__GetVideoSourceConfigurationOptions, struct afterveda__AnyResponse& trt__GetVideoSourceConfigurationOptionsResponse);
//gsoap trt service method-action: GetVideoEncoderConfigurationOptions http://www.onvif.org/ver10/media/wsdl/GetVideoEncoderConfigurationOptions
int __trt__GetVideoEncoderConfigurationOptions(struct afterveda__EmptyRequest* trt__GetVideoEncoderConfigurationOptions, struct afterveda__AnyResponse& trt__GetVideoEncoderConfigurationOptionsResponse);
//gsoap trt service method-action: SetVideoEncoderConfiguration http://www.onvif.org/ver10/media/wsdl/SetVideoEncoderConfiguration
int __trt__SetVideoEncoderConfiguration(struct afterveda__EmptyRequest* trt__SetVideoEncoderConfiguration, struct afterveda__AnyResponse& trt__SetVideoEncoderConfigurationResponse);

//gsoap tr2 service name: Media2Binding
//gsoap tr2 service namespace: http://www.onvif.org/ver20/media/wsdl
//gsoap tr2 service style: document
//gsoap tr2 service encoding: literal
//gsoap tr2 service method-action: GetProfiles http://www.onvif.org/ver20/media/wsdl/GetProfiles
int __tr2__GetProfiles(struct afterveda__EmptyRequest* tr2__GetProfiles, struct afterveda__AnyResponse& tr2__GetProfilesResponse);
//gsoap tr2 service method-action: GetStreamUri http://www.onvif.org/ver20/media/wsdl/GetStreamUri
int __tr2__GetStreamUri(struct afterveda__EmptyRequest* tr2__GetStreamUri, struct afterveda__AnyResponse& tr2__GetStreamUriResponse);
//gsoap tr2 service method-action: GetServiceCapabilities http://www.onvif.org/ver20/media/wsdl/GetServiceCapabilities
int __tr2__GetServiceCapabilities(struct afterveda__EmptyRequest* tr2__GetServiceCapabilities, struct afterveda__AnyResponse& tr2__GetServiceCapabilitiesResponse);

//gsoap tptz service name: PTZBinding
//gsoap tptz service namespace: http://www.onvif.org/ver20/ptz/wsdl
//gsoap tptz service style: document
//gsoap tptz service encoding: literal
//gsoap tptz service method-action: GetNodes http://www.onvif.org/ver20/ptz/wsdl/GetNodes
int __tptz__GetNodes(struct afterveda__EmptyRequest* tptz__GetNodes, struct afterveda__AnyResponse& tptz__GetNodesResponse);
//gsoap tptz service method-action: GetConfigurations http://www.onvif.org/ver20/ptz/wsdl/GetConfigurations
int __tptz__GetConfigurations(struct afterveda__EmptyRequest* tptz__GetConfigurations, struct afterveda__AnyResponse& tptz__GetConfigurationsResponse);
//gsoap tptz service method-action: GetPresets http://www.onvif.org/ver20/ptz/wsdl/GetPresets
int __tptz__GetPresets(struct afterveda__EmptyRequest* tptz__GetPresets, struct afterveda__AnyResponse& tptz__GetPresetsResponse);
//gsoap tptz service method-action: SetPreset http://www.onvif.org/ver20/ptz/wsdl/SetPreset
int __tptz__SetPreset(struct afterveda__EmptyRequest* tptz__SetPreset, struct afterveda__AnyResponse& tptz__SetPresetResponse);
//gsoap tptz service method-action: GotoPreset http://www.onvif.org/ver20/ptz/wsdl/GotoPreset
int __tptz__GotoPreset(struct afterveda__EmptyRequest* tptz__GotoPreset, struct afterveda__AnyResponse& tptz__GotoPresetResponse);
//gsoap tptz service method-action: GetStatus http://www.onvif.org/ver20/ptz/wsdl/GetStatus
int __tptz__GetStatus(struct _tptz__GetStatus* tptz__GetStatus, struct _tptz__GetStatusResponse& tptz__GetStatusResponse);
//gsoap tptz service method-action: GotoHomePosition http://www.onvif.org/ver20/ptz/wsdl/GotoHomePosition
int __tptz__GotoHomePosition(struct afterveda__EmptyRequest* tptz__GotoHomePosition, struct afterveda__AnyResponse& tptz__GotoHomePositionResponse);
//gsoap tptz service method-action: SetHomePosition http://www.onvif.org/ver20/ptz/wsdl/SetHomePosition
int __tptz__SetHomePosition(struct afterveda__EmptyRequest* tptz__SetHomePosition, struct afterveda__AnyResponse& tptz__SetHomePositionResponse);
//gsoap tptz service method-action: ContinuousMove http://www.onvif.org/ver20/ptz/wsdl/ContinuousMove
int __tptz__ContinuousMove(struct afterveda__EmptyRequest* tptz__ContinuousMove, struct afterveda__AnyResponse& tptz__ContinuousMoveResponse);
//gsoap tptz service method-action: RelativeMove http://www.onvif.org/ver20/ptz/wsdl/RelativeMove
int __tptz__RelativeMove(struct afterveda__EmptyRequest* tptz__RelativeMove, struct afterveda__AnyResponse& tptz__RelativeMoveResponse);
//gsoap tptz service method-action: Stop http://www.onvif.org/ver20/ptz/wsdl/Stop
int __tptz__Stop(struct afterveda__EmptyRequest* tptz__Stop, struct afterveda__AnyResponse& tptz__StopResponse);

//gsoap timg service name: ImagingBinding
//gsoap timg service namespace: http://www.onvif.org/ver20/imaging/wsdl
//gsoap timg service style: document
//gsoap timg service encoding: literal
//gsoap timg service method-action: GetServiceCapabilities http://www.onvif.org/ver20/imaging/wsdl/GetServiceCapabilities
int __timg__GetServiceCapabilities(struct afterveda__EmptyRequest* timg__GetServiceCapabilities, struct afterveda__AnyResponse& timg__GetServiceCapabilitiesResponse);
//gsoap timg service method-action: GetImagingSettings http://www.onvif.org/ver20/imaging/wsdl/GetImagingSettings
int __timg__GetImagingSettings(struct afterveda__EmptyRequest* timg__GetImagingSettings, struct afterveda__AnyResponse& timg__GetImagingSettingsResponse);
//gsoap timg service method-action: SetImagingSettings http://www.onvif.org/ver20/imaging/wsdl/SetImagingSettings
int __timg__SetImagingSettings(struct afterveda__EmptyRequest* timg__SetImagingSettings, struct afterveda__AnyResponse& timg__SetImagingSettingsResponse);
//gsoap timg service method-action: GetOptions http://www.onvif.org/ver20/imaging/wsdl/GetOptions
int __timg__GetOptions(struct afterveda__EmptyRequest* timg__GetOptions, struct afterveda__AnyResponse& timg__GetOptionsResponse);

//gsoap tev service name: EventsBinding
//gsoap tev service namespace: http://www.onvif.org/ver10/events/wsdl
//gsoap tev service style: document
//gsoap tev service encoding: literal
//gsoap tev service method-action: GetEventProperties http://www.onvif.org/ver10/events/wsdl/GetEventProperties
int __tev__GetEventProperties(struct afterveda__EmptyRequest* tev__GetEventProperties, struct afterveda__AnyResponse& tev__GetEventPropertiesResponse);
//gsoap tev service method-action: CreatePullPointSubscription http://www.onvif.org/ver10/events/wsdl/CreatePullPointSubscription
int __tev__CreatePullPointSubscription(struct afterveda__EmptyRequest* tev__CreatePullPointSubscription, struct afterveda__AnyResponse& tev__CreatePullPointSubscriptionResponse);
//gsoap tev service method-action: PullMessages http://www.onvif.org/ver10/events/wsdl/PullMessages
int __tev__PullMessages(struct afterveda__EmptyRequest* tev__PullMessages, struct afterveda__AnyResponse& tev__PullMessagesResponse);

//gsoap wsnt service name: NotificationBinding
//gsoap wsnt service namespace: http://docs.oasis-open.org/wsn/b-2
//gsoap wsnt service style: document
//gsoap wsnt service encoding: literal
//gsoap wsnt service method-action: Renew http://docs.oasis-open.org/wsn/bw-2/SubscriptionManager/RenewRequest
int __wsnt__Renew(struct afterveda__EmptyRequest* wsnt__Renew, struct afterveda__AnyResponse& wsnt__RenewResponse);
//gsoap wsnt service method-action: Unsubscribe http://docs.oasis-open.org/wsn/bw-2/SubscriptionManager/UnsubscribeRequest
int __wsnt__Unsubscribe(struct afterveda__EmptyRequest* wsnt__Unsubscribe, struct afterveda__AnyResponse& wsnt__UnsubscribeResponse);

//gsoap tosd service name: OSDConfigurationBinding
//gsoap tosd service namespace: http://www.onvif.org/ver10/osd/wsdl
//gsoap tosd service style: document
//gsoap tosd service encoding: literal
//gsoap tosd service method-action: GetServiceCapabilities http://www.onvif.org/ver10/osd/wsdl/GetServiceCapabilities
int __tosd__GetServiceCapabilities(struct afterveda__EmptyRequest* tosd__GetServiceCapabilities, struct afterveda__AnyResponse& tosd__GetServiceCapabilitiesResponse);
//gsoap tosd service method-action: GetOSDs http://www.onvif.org/ver10/osd/wsdl/GetOSDs
int __tosd__GetOSDs(struct afterveda__EmptyRequest* tosd__GetOSDs, struct afterveda__AnyResponse& tosd__GetOSDsResponse);
//gsoap tosd service method-action: GetOSD http://www.onvif.org/ver10/osd/wsdl/GetOSD
int __tosd__GetOSD(struct afterveda__EmptyRequest* tosd__GetOSD, struct afterveda__AnyResponse& tosd__GetOSDResponse);
//gsoap tosd service method-action: SetOSD http://www.onvif.org/ver10/osd/wsdl/SetOSD
int __tosd__SetOSD(struct afterveda__EmptyRequest* tosd__SetOSD, struct afterveda__AnyResponse& tosd__SetOSDResponse);
