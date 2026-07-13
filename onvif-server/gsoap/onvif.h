#import "soap12.h"
#import "wsse.h"
#import "wsdd10.h"
#import "wsa.h"

//gsoap tt schema namespace: http://www.onvif.org/ver10/schema
//gsoap tt schema elementForm: qualified
//gsoap tt schema attributeForm: unqualified
//gsoap tds schema elementForm: qualified
//gsoap tds schema attributeForm: unqualified
//gsoap trt schema elementForm: qualified
//gsoap trt schema attributeForm: unqualified
//gsoap tr2 schema elementForm: qualified
//gsoap tr2 schema attributeForm: unqualified
//gsoap tptz schema elementForm: qualified
//gsoap tptz schema attributeForm: unqualified
//gsoap timg schema elementForm: qualified
//gsoap timg schema attributeForm: unqualified
//gsoap wstop schema namespace: http://docs.oasis-open.org/wsn/t-1
//gsoap tns1 schema namespace: http://www.onvif.org/ver10/topics

struct _tds__GetServices {
    bool IncludeCapability;
};

struct _tds__GetServiceCapabilities {};

struct _tds__GetDeviceInformation {};

struct _tds__GetSystemDateAndTime {};

struct _tds__GetScopes {};

struct _tds__GetHostname {};

struct _tds__GetNetworkInterfaces {};

struct _tds__GetUsers {};

enum tds__CapabilityCategory {
    tds__CapabilityCategory__All,
    tds__CapabilityCategory__Analytics,
    tds__CapabilityCategory__Device,
    tds__CapabilityCategory__Events,
    tds__CapabilityCategory__Imaging,
    tds__CapabilityCategory__Media,
    tds__CapabilityCategory__PTZ
};

struct _tds__GetCapabilities {
    int __sizeCategory;
    enum tds__CapabilityCategory* Category;
};

struct _tds__GetDeviceInformationResponse {
    char* Manufacturer;
    char* Model;
    char* FirmwareVersion;
    char* SerialNumber;
    char* HardwareId;
};

struct tt__OnvifVersion {
    int Major;
    int Minor;
};

struct tds__Service {
    char* Namespace;
    char* XAddr;
    struct tt__OnvifVersion* Version;
};

struct tt__StreamingCapabilities {
    bool* RTPMulticast;
    bool* RTP_USCORETCP;
    bool* RTP_USCORERTSP_USCORETCP;
};

struct tt__DeviceCapabilities {
    char* XAddr;
};

struct tt__MediaCapabilities {
    char* XAddr;
    struct tt__StreamingCapabilities* StreamingCapabilities;
};

struct tt__PTZCapabilities {
    char* XAddr;
};

struct tt__ImagingCapabilities {
    char* XAddr;
};

struct tt__Capabilities {
    struct tt__DeviceCapabilities* Device;
    struct tt__MediaCapabilities* Media;
    struct tt__MediaCapabilities* Media2;
    struct tt__PTZCapabilities* PTZ;
    struct tt__ImagingCapabilities* Imaging;
};

struct _tds__GetServicesResponse {
    int __sizeService;
    struct tds__Service* Service;
};

struct _tds__GetCapabilitiesResponse {
    struct tt__Capabilities* Capabilities;
};

struct tds__NetworkCapabilities {
    @ bool IPFilter;
    @ bool ZeroConfiguration;
    @ bool IPVersion6;
    @ bool DynDNS;
};

struct tds__SecurityCapabilities {
    @ bool OnboardKeyGeneration;
    @ bool AccessPolicyConfig;
    @ bool DefaultAccessPolicy;
    @ bool Dot1X;
    @ bool RemoteUserHandling;
};

struct tds__SystemCapabilities {
    @ bool DiscoveryResolve;
    @ bool DiscoveryBye;
    @ bool RemoteDiscovery;
    @ bool SystemBackup;
    @ bool SystemLogging;
    @ bool FirmwareUpgrade;
};

struct tds__DeviceServiceCapabilities {
    struct tds__NetworkCapabilities* Network;
    struct tds__SecurityCapabilities* Security;
    struct tds__SystemCapabilities* System;
};

struct _tds__GetServiceCapabilitiesResponse {
    struct tds__DeviceServiceCapabilities* Capabilities;
};

struct tt__Time {
    int Hour;
    int Minute;
    int Second;
};

struct tt__Date {
    int Year;
    int Month;
    int Day;
};

struct tt__DateTime {
    struct tt__Time* Time;
    struct tt__Date* Date;
};

struct tt__SystemDateTime {
    char* DateTimeType;
    bool DaylightSavings;
    struct tt__DateTime* UTCDateTime;
};

struct _tds__GetSystemDateAndTimeResponse {
    struct tt__SystemDateTime* SystemDateAndTime;
};

struct tt__Scope {
    char* ScopeDef;
    char* ScopeItem;
};

struct _tds__GetScopesResponse {
    int __sizeScopes;
    struct tt__Scope* Scopes;
};

struct tt__HostnameInformation {
    bool FromDHCP;
    char* Name;
};

struct _tds__GetHostnameResponse {
    struct tt__HostnameInformation* HostnameInformation;
};

struct tt__NetworkInterfaceInfo {
    char* Name;
    char* HwAddress;
    int MTU;
};

struct tt__PrefixedIPv4Address {
    char* Address;
    int PrefixLength;
};

struct tt__IPv4Configuration {
    int __sizeManual;
    struct tt__PrefixedIPv4Address* Manual;
    struct tt__PrefixedIPv4Address* LinkLocal;
    struct tt__PrefixedIPv4Address* FromDHCP;
    bool DHCP;
};

struct tt__IPv4NetworkInterface {
    bool Enabled;
    struct tt__IPv4Configuration* Config;
};

struct tt__NetworkInterface {
    @ char* token;
    @ bool enabled;
    struct tt__NetworkInterfaceInfo* Info;
    struct tt__IPv4NetworkInterface* IPv4;
};

struct _tds__GetNetworkInterfacesResponse {
    int __sizeNetworkInterfaces;
    struct tt__NetworkInterface* NetworkInterfaces;
};

struct tt__User {
    char* Username;
    char* UserLevel;
};

struct _tds__GetUsersResponse {
    int __sizeUser;
    struct tt__User* User;
};

struct tt__IntRectangle {
    @ int x;
    @ int y;
    @ int width;
    @ int height;
};

struct tt__VideoResolution {
    int Width;
    int Height;
};

struct tt__VideoRateControl {
    int FrameRateLimit;
    int EncodingInterval;
    int BitrateLimit;
};

struct tt__H264Configuration {
    int GovLength;
    char* H264Profile;
};

struct tt__IPAddress {
    char* Type;
    char* IPv4Address;
};

struct tt__MulticastConfiguration {
    struct tt__IPAddress* Address;
    int Port;
    int TTL;
    bool AutoStart;
};

struct tt__VideoSource {
    @ char* token;
    float Framerate;
    struct tt__VideoResolution* Resolution;
};

struct tt__VideoSourceConfiguration {
    @ char* token;
    char* Name;
    int UseCount;
    char* SourceToken;
    struct tt__IntRectangle* Bounds;
};

struct tt__VideoEncoderConfiguration {
    @ char* token;
    char* Name;
    int UseCount;
    char* Encoding;
    struct tt__VideoResolution* Resolution;
    float Quality;
    struct tt__VideoRateControl* RateControl;
    struct tt__H264Configuration* H264;
    struct tt__MulticastConfiguration* Multicast;
    char* SessionTimeout;
};

struct tt__FloatRange {
    float Min;
    float Max;
};

struct tt__IntRange {
    int Min;
    int Max;
};

struct tt__IntRectangleRange {
    struct tt__IntRange* XRange;
    struct tt__IntRange* YRange;
    struct tt__IntRange* WidthRange;
    struct tt__IntRange* HeightRange;
};

struct tt__VideoSourceConfigurationOptions {
    struct tt__IntRectangleRange* BoundsRange;
    int __sizeVideoSourceTokensAvailable;
    char** VideoSourceTokensAvailable;
};

struct tt__H264Options {
    int __sizeResolutionsAvailable;
    struct tt__VideoResolution* ResolutionsAvailable;
    struct tt__IntRange* GovLengthRange;
    struct tt__IntRange* FrameRateRange;
    struct tt__IntRange* EncodingIntervalRange;
    int __sizeH264ProfilesSupported;
    char** H264ProfilesSupported;
};

struct tt__VideoEncoderConfigurationOptionsExtension {
    struct tt__H264Options* H264;
};

struct tt__VideoEncoderConfigurationOptions {
    struct tt__FloatRange* QualityRange;
    struct tt__H264Options* H264;
    struct tt__VideoEncoderConfigurationOptionsExtension* Extension;
    struct tt__IntRange* BitrateRange;
};

struct tt__Transport {
    char* Protocol;
};

struct tt__StreamSetup {
    char* Stream;
    struct tt__Transport* Transport;
};

struct _trt__GetProfiles {};

struct _trt__GetProfile {
    char* ProfileToken;
};

struct _trt__GetServiceCapabilities {};

struct _trt__GetVideoSources {};

struct _trt__GetVideoSourceConfigurations {};

struct _trt__GetVideoSourceConfiguration {
    char* ConfigurationToken;
};

struct _trt__GetVideoEncoderConfigurations {};

struct _trt__GetStreamUri {
    struct tt__StreamSetup* StreamSetup;
    char* ProfileToken;
};

struct _trt__GetVideoEncoderConfiguration {
    char* ConfigurationToken;
};

struct _trt__GetVideoSourceConfigurationOptions {
    char* ConfigurationToken;
    char* ProfileToken;
};

struct _trt__GetVideoEncoderConfigurationOptions {
    char* ConfigurationToken;
    char* ProfileToken;
};

struct trt__Capabilities {
    @ bool SnapshotUri;
    @ bool Rotation;
    @ bool VideoSourceMode;
    @ bool OSD;
    @ bool TemporaryOSDText;
    @ bool EXICompression;
};

struct _trt__GetServiceCapabilitiesResponse {
    struct trt__Capabilities* Capabilities;
};

struct _trt__GetVideoSourceConfigurationsResponse {
    int __sizeConfigurations;
    struct tt__VideoSourceConfiguration* Configurations;
};

struct _trt__GetVideoSourceConfigurationResponse {
    struct tt__VideoSourceConfiguration* Configuration;
};

struct _trt__GetVideoSourceConfigurationOptionsResponse {
    struct tt__VideoSourceConfigurationOptions* Options;
};

struct _trt__GetVideoEncoderConfigurationOptionsResponse {
    struct tt__VideoEncoderConfigurationOptions* Options;
};

struct _trt__SetVideoEncoderConfiguration {
    struct tt__VideoEncoderConfiguration* Configuration;
    bool ForcePersistence;
};

struct _trt__SetVideoEncoderConfigurationResponse {
};

struct tr2__Capabilities {
    @ bool SnapshotUri;
    @ bool Rotation;
    @ bool VideoSourceMode;
    @ bool OSD;
    @ bool TemporaryOSDText;
    @ bool EXICompression;
};

struct tr2__ConfigurationSet {
    struct tt__VideoSourceConfiguration* VideoSource;
    struct tt__VideoEncoderConfiguration* VideoEncoder;
};

struct tr2__MediaProfile {
    @ char* token;
    @ bool fixed;
    char* Name;
    struct tr2__ConfigurationSet* Configurations;
};

struct _tr2__GetProfiles {
    char* Token;
    int __sizeType;
    char** Type;
};

struct _tr2__GetStreamUri {
    char* Protocol;
    char* ProfileToken;
};

struct _tr2__GetServiceCapabilities {};

struct _tr2__GetProfilesResponse {
    int __sizeProfiles;
    struct tr2__MediaProfile* Profiles;
};

struct _tr2__GetStreamUriResponse {
    char* Uri;
};

struct _tr2__GetServiceCapabilitiesResponse {
    struct tr2__Capabilities* Capabilities;
};

struct tt__Space2DDescription {
    char* URI;
    struct tt__FloatRange* XRange;
    struct tt__FloatRange* YRange;
};

struct tt__PTZSpaces {
    int __sizeRelativePanTiltTranslationSpace;
    struct tt__Space2DDescription* RelativePanTiltTranslationSpace;
    int __sizeContinuousPanTiltVelocitySpace;
    struct tt__Space2DDescription* ContinuousPanTiltVelocitySpace;
};

struct tptz__Capabilities {
    @ bool EFlip;
    @ bool Reverse;
    @ bool GetCompatibleConfigurations;
    @ bool MoveStatus;
    @ bool StatusPosition;
};

struct tt__DurationRange {
    char* Min;
    char* Max;
};

struct tt__PTZConfigurationOptions {
    struct tt__PTZSpaces* Spaces;
    struct tt__DurationRange* PTZTimeout;
};

struct tt__PTZNode {
    @ char* token;
    char* Name;
    struct tt__PTZSpaces* SupportedPTZSpaces;
    int MaximumNumberOfPresets;
    bool HomeSupported;
};

struct tt__PanTiltLimits {
    struct tt__Space2DDescription* Range;
};

struct tt__PTZConfiguration {
    @ char* token;
    char* Name;
    int UseCount;
    char* NodeToken;
    char* DefaultContinuousPanTiltVelocitySpace;
    char* DefaultRelativePanTiltTranslationSpace;
    char* DefaultPTZTimeout;
    struct tt__PanTiltLimits* PanTiltLimits;
};

struct tt__PTZPreset {
    @ char* token;
    char* Name;
};

struct _tptz__GetNodesResponse {
    int __sizePTZNode;
    struct tt__PTZNode* PTZNode;
};

struct _tptz__GetServiceCapabilities {};

struct _tptz__GetServiceCapabilitiesResponse {
    struct tptz__Capabilities* Capabilities;
};

struct _tptz__GetNode {
    char* NodeToken;
};

struct _tptz__GetNodeResponse {
    struct tt__PTZNode* PTZNode;
};

struct _tptz__GetConfiguration {
    char* PTZConfigurationToken;
};

struct _tptz__GetConfigurationResponse {
    struct tt__PTZConfiguration* PTZConfiguration;
};

struct _tptz__GetConfigurationOptions {
    char* ConfigurationToken;
};

struct _tptz__GetConfigurationOptionsResponse {
    struct tt__PTZConfigurationOptions* PTZConfigurationOptions;
};

struct _tptz__GetConfigurationsResponse {
    int __sizePTZConfiguration;
    struct tt__PTZConfiguration* PTZConfiguration;
};

struct _tptz__GetPresetsResponse {
    int __sizePreset;
    struct tt__PTZPreset* Preset;
};

struct _tptz__GetNodes {};

struct _tptz__GetConfigurations {};

struct _tptz__GetPresets {
    char* ProfileToken;
};

struct _tptz__SetPreset {
    char* ProfileToken;
    char* PresetName;
    char* PresetToken;
};

struct _tptz__SetPresetResponse {
    char* PresetToken;
};

struct tt__Profile {
    @ char* token;
    @ bool fixed;
    char* Name;
    struct tt__VideoSourceConfiguration* VideoSourceConfiguration;
    struct tt__VideoEncoderConfiguration* VideoEncoderConfiguration;
    struct tt__PTZConfiguration* PTZConfiguration;
};

struct tt__MediaUri {
    char* Uri;
    bool InvalidAfterConnect;
    bool InvalidAfterReboot;
    char* Timeout;
};

struct _trt__GetProfilesResponse {
    int __sizeProfiles;
    struct tt__Profile* Profiles;
};

struct _trt__GetProfileResponse {
    struct tt__Profile* Profile;
};

struct _trt__GetStreamUriResponse {
    struct tt__MediaUri* MediaUri;
};

struct _trt__GetVideoSourcesResponse {
    int __sizeVideoSources;
    struct tt__VideoSource* VideoSources;
};

struct _trt__GetVideoEncoderConfigurationResponse {
    struct tt__VideoEncoderConfiguration* Configuration;
};

struct _trt__GetVideoEncoderConfigurationsResponse {
    int __sizeConfigurations;
    struct tt__VideoEncoderConfiguration* Configurations;
};

struct _tptz__GetStatus {
    char* ProfileToken;
};

struct tt__Vector2D {
    @ double x;
    @ double y;
};

struct tt__Vector1D {
    @ double x;
};

struct tt__PTZVector {
    struct tt__Vector2D* PanTilt;
    struct tt__Vector1D* Zoom;
};

struct tt__PTZSpeed {
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

struct _tptz__ContinuousMove {
    char* ProfileToken;
    struct tt__PTZSpeed* Velocity;
    char* Timeout;
};

struct _tptz__RelativeMove {
    char* ProfileToken;
    struct tt__PTZVector* Translation;
    struct tt__PTZSpeed* Speed;
};

struct _tptz__GotoPreset {
    char* ProfileToken;
    char* PresetToken;
    struct tt__PTZSpeed* Speed;
};

struct _tptz__GotoPresetResponse {};

struct _tptz__SetHomePosition {
    char* ProfileToken;
};

struct _tptz__SetHomePositionResponse {};

struct _tptz__GotoHomePosition {
    char* ProfileToken;
    struct tt__PTZSpeed* Speed;
};

struct _tptz__GotoHomePositionResponse {};

struct _tptz__ContinuousMoveResponse {};

struct _tptz__RelativeMoveResponse {};

struct _tptz__Stop {
    char* ProfileToken;
    bool* PanTilt;
    bool* Zoom;
};

struct _tptz__StopResponse {};

struct tt__ImagingSettings20 {
    float* Brightness;
    float* ColorSaturation;
    float* Contrast;
};

struct tt__ImagingOptions20 {
    struct tt__FloatRange* Brightness;
    struct tt__FloatRange* ColorSaturation;
    struct tt__FloatRange* Contrast;
};

struct timg__Capabilities {
    @ bool ImageStabilization;
    @ bool Presets;
};

struct _timg__GetServiceCapabilities {};

struct _timg__GetServiceCapabilitiesResponse {
    struct timg__Capabilities* Capabilities;
};

struct _timg__GetImagingSettings {
    char* VideoSourceToken;
};

struct _timg__GetImagingSettingsResponse {
    struct tt__ImagingSettings20* ImagingSettings;
};

struct _timg__SetImagingSettings {
    char* VideoSourceToken;
    struct tt__ImagingSettings20* ImagingSettings;
    bool ForcePersistence;
};

struct _timg__SetImagingSettingsResponse {};

struct _timg__GetOptions {
    char* VideoSourceToken;
};

struct _timg__GetOptionsResponse {
    struct tt__ImagingOptions20* ImagingOptions;
};

struct tt__OSDPosConfiguration {
    char* Type;
};

struct tt__OSDTextConfiguration {
    char* Type;
    char* PlainText;
};

struct tt__OSDConfiguration {
    @ char* token;
    char* VideoSourceConfigurationToken;
    char* Type;
    struct tt__OSDPosConfiguration* Position;
    struct tt__OSDTextConfiguration* TextString;
};

struct tt__MaximumNumberOfOSDs {
    @ int Total;
    @ int* Image;
    @ int* PlainText;
    @ int* Date;
    @ int* Time;
    @ int* DateAndTime;
};

struct tt__OSDTextOptions {
    int __sizeType;
    char** Type;
};

struct tt__OSDConfigurationOptions {
    struct tt__MaximumNumberOfOSDs* MaximumNumberOfOSDs;
    int __sizeType;
    char** Type;
    int __sizePositionOption;
    char** PositionOption;
    struct tt__OSDTextOptions* TextOption;
};

struct _trt__GetOSDs {
    char* ConfigurationToken;
};

struct _trt__GetOSDsResponse {
    int __sizeOSDs;
    struct tt__OSDConfiguration* OSDs;
};

struct _trt__GetOSD {
    char* OSDToken;
};

struct _trt__GetOSDResponse {
    struct tt__OSDConfiguration* OSD;
};

struct _trt__GetOSDOptions {
    char* ConfigurationToken;
};

struct _trt__GetOSDOptionsResponse {
    struct tt__OSDConfigurationOptions* OSDOptions;
};

struct _trt__SetOSD {
    struct tt__OSDConfiguration* OSD;
};

struct _trt__SetOSDResponse {};

struct _trt__CreateOSD {
    struct tt__OSDConfiguration* OSD;
};

struct _trt__CreateOSDResponse {
    char* OSDToken;
};

struct _trt__DeleteOSD {
    char* OSDToken;
};

struct _trt__DeleteOSDResponse {};

//gsoap tds service name: DeviceBinding
//gsoap tds service namespace: http://www.onvif.org/ver10/device/wsdl
//gsoap tds service style: document
//gsoap tds service encoding: literal
//gsoap tds service method-action: GetServices http://www.onvif.org/ver10/device/wsdl/GetServices
int __tds__GetServices(struct _tds__GetServices* tds__GetServices, struct _tds__GetServicesResponse& tds__GetServicesResponse);
//gsoap tds service method-action: GetServiceCapabilities http://www.onvif.org/ver10/device/wsdl/GetServiceCapabilities
int __tds__GetServiceCapabilities(struct _tds__GetServiceCapabilities* tds__GetServiceCapabilities, struct _tds__GetServiceCapabilitiesResponse& tds__GetServiceCapabilitiesResponse);
//gsoap tds service method-action: GetDeviceInformation http://www.onvif.org/ver10/device/wsdl/GetDeviceInformation
int __tds__GetDeviceInformation(struct _tds__GetDeviceInformation* tds__GetDeviceInformation, struct _tds__GetDeviceInformationResponse& tds__GetDeviceInformationResponse);
//gsoap tds service method-action: GetSystemDateAndTime http://www.onvif.org/ver10/device/wsdl/GetSystemDateAndTime
int __tds__GetSystemDateAndTime(struct _tds__GetSystemDateAndTime* tds__GetSystemDateAndTime, struct _tds__GetSystemDateAndTimeResponse& tds__GetSystemDateAndTimeResponse);
//gsoap tds service method-action: GetScopes http://www.onvif.org/ver10/device/wsdl/GetScopes
int __tds__GetScopes(struct _tds__GetScopes* tds__GetScopes, struct _tds__GetScopesResponse& tds__GetScopesResponse);
//gsoap tds service method-action: GetHostname http://www.onvif.org/ver10/device/wsdl/GetHostname
int __tds__GetHostname(struct _tds__GetHostname* tds__GetHostname, struct _tds__GetHostnameResponse& tds__GetHostnameResponse);
//gsoap tds service method-action: GetNetworkInterfaces http://www.onvif.org/ver10/device/wsdl/GetNetworkInterfaces
int __tds__GetNetworkInterfaces(struct _tds__GetNetworkInterfaces* tds__GetNetworkInterfaces, struct _tds__GetNetworkInterfacesResponse& tds__GetNetworkInterfacesResponse);
//gsoap tds service method-action: GetUsers http://www.onvif.org/ver10/device/wsdl/GetUsers
int __tds__GetUsers(struct _tds__GetUsers* tds__GetUsers, struct _tds__GetUsersResponse& tds__GetUsersResponse);
//gsoap tds service method-action: GetCapabilities http://www.onvif.org/ver10/device/wsdl/GetCapabilities
int __tds__GetCapabilities(struct _tds__GetCapabilities* tds__GetCapabilities, struct _tds__GetCapabilitiesResponse& tds__GetCapabilitiesResponse);

//gsoap trt service name: MediaBinding
//gsoap trt service namespace: http://www.onvif.org/ver10/media/wsdl
//gsoap trt service style: document
//gsoap trt service encoding: literal
//gsoap trt service method-action: GetProfiles http://www.onvif.org/ver10/media/wsdl/GetProfiles
int __trt__GetProfiles(struct _trt__GetProfiles* trt__GetProfiles, struct _trt__GetProfilesResponse& trt__GetProfilesResponse);
//gsoap trt service method-action: GetProfile http://www.onvif.org/ver10/media/wsdl/GetProfile
int __trt__GetProfile(struct _trt__GetProfile* trt__GetProfile, struct _trt__GetProfileResponse& trt__GetProfileResponse);
//gsoap trt service method-action: GetServiceCapabilities http://www.onvif.org/ver10/media/wsdl/GetServiceCapabilities
int __trt__GetServiceCapabilities(struct _trt__GetServiceCapabilities* trt__GetServiceCapabilities, struct _trt__GetServiceCapabilitiesResponse& trt__GetServiceCapabilitiesResponse);
//gsoap trt service method-action: GetVideoSources http://www.onvif.org/ver10/media/wsdl/GetVideoSources
int __trt__GetVideoSources(struct _trt__GetVideoSources* trt__GetVideoSources, struct _trt__GetVideoSourcesResponse& trt__GetVideoSourcesResponse);
//gsoap trt service method-action: GetOSDs http://www.onvif.org/ver10/media/wsdl/GetOSDs
int __trt__GetOSDs(struct _trt__GetOSDs* trt__GetOSDs, struct _trt__GetOSDsResponse& trt__GetOSDsResponse);
//gsoap trt service method-action: GetOSD http://www.onvif.org/ver10/media/wsdl/GetOSD
int __trt__GetOSD(struct _trt__GetOSD* trt__GetOSD, struct _trt__GetOSDResponse& trt__GetOSDResponse);
//gsoap trt service method-action: GetOSDOptions http://www.onvif.org/ver10/media/wsdl/GetOSDOptions
int __trt__GetOSDOptions(struct _trt__GetOSDOptions* trt__GetOSDOptions, struct _trt__GetOSDOptionsResponse& trt__GetOSDOptionsResponse);
//gsoap trt service method-action: SetOSD http://www.onvif.org/ver10/media/wsdl/SetOSD
int __trt__SetOSD(struct _trt__SetOSD* trt__SetOSD, struct _trt__SetOSDResponse& trt__SetOSDResponse);
//gsoap trt service method-action: CreateOSD http://www.onvif.org/ver10/media/wsdl/CreateOSD
int __trt__CreateOSD(struct _trt__CreateOSD* trt__CreateOSD, struct _trt__CreateOSDResponse& trt__CreateOSDResponse);
//gsoap trt service method-action: DeleteOSD http://www.onvif.org/ver10/media/wsdl/DeleteOSD
int __trt__DeleteOSD(struct _trt__DeleteOSD* trt__DeleteOSD, struct _trt__DeleteOSDResponse& trt__DeleteOSDResponse);
//gsoap trt service method-action: GetVideoSourceConfigurations http://www.onvif.org/ver10/media/wsdl/GetVideoSourceConfigurations
int __trt__GetVideoSourceConfigurations(struct _trt__GetVideoSourceConfigurations* trt__GetVideoSourceConfigurations, struct _trt__GetVideoSourceConfigurationsResponse& trt__GetVideoSourceConfigurationsResponse);
//gsoap trt service method-action: GetVideoSourceConfiguration http://www.onvif.org/ver10/media/wsdl/GetVideoSourceConfiguration
int __trt__GetVideoSourceConfiguration(struct _trt__GetVideoSourceConfiguration* trt__GetVideoSourceConfiguration, struct _trt__GetVideoSourceConfigurationResponse& trt__GetVideoSourceConfigurationResponse);
//gsoap trt service method-action: GetVideoEncoderConfigurations http://www.onvif.org/ver10/media/wsdl/GetVideoEncoderConfigurations
int __trt__GetVideoEncoderConfigurations(struct _trt__GetVideoEncoderConfigurations* trt__GetVideoEncoderConfigurations, struct _trt__GetVideoEncoderConfigurationsResponse& trt__GetVideoEncoderConfigurationsResponse);
//gsoap trt service method-action: GetStreamUri http://www.onvif.org/ver10/media/wsdl/GetStreamUri
int __trt__GetStreamUri(struct _trt__GetStreamUri* trt__GetStreamUri, struct _trt__GetStreamUriResponse& trt__GetStreamUriResponse);
//gsoap trt service method-action: GetVideoEncoderConfiguration http://www.onvif.org/ver10/media/wsdl/GetVideoEncoderConfiguration
int __trt__GetVideoEncoderConfiguration(struct _trt__GetVideoEncoderConfiguration* trt__GetVideoEncoderConfiguration, struct _trt__GetVideoEncoderConfigurationResponse& trt__GetVideoEncoderConfigurationResponse);
//gsoap trt service method-action: GetVideoSourceConfigurationOptions http://www.onvif.org/ver10/media/wsdl/GetVideoSourceConfigurationOptions
int __trt__GetVideoSourceConfigurationOptions(struct _trt__GetVideoSourceConfigurationOptions* trt__GetVideoSourceConfigurationOptions, struct _trt__GetVideoSourceConfigurationOptionsResponse& trt__GetVideoSourceConfigurationOptionsResponse);
//gsoap trt service method-action: GetVideoEncoderConfigurationOptions http://www.onvif.org/ver10/media/wsdl/GetVideoEncoderConfigurationOptions
int __trt__GetVideoEncoderConfigurationOptions(struct _trt__GetVideoEncoderConfigurationOptions* trt__GetVideoEncoderConfigurationOptions, struct _trt__GetVideoEncoderConfigurationOptionsResponse& trt__GetVideoEncoderConfigurationOptionsResponse);
//gsoap trt service method-action: SetVideoEncoderConfiguration http://www.onvif.org/ver10/media/wsdl/SetVideoEncoderConfiguration
int __trt__SetVideoEncoderConfiguration(struct _trt__SetVideoEncoderConfiguration* trt__SetVideoEncoderConfiguration, struct _trt__SetVideoEncoderConfigurationResponse& trt__SetVideoEncoderConfigurationResponse);

//gsoap tr2 service name: Media2Binding
//gsoap tr2 service namespace: http://www.onvif.org/ver20/media/wsdl
//gsoap tr2 service style: document
//gsoap tr2 service encoding: literal
//gsoap tr2 service method-action: GetProfiles http://www.onvif.org/ver20/media/wsdl/GetProfiles
int __tr2__GetProfiles(struct _tr2__GetProfiles* tr2__GetProfiles, struct _tr2__GetProfilesResponse& tr2__GetProfilesResponse);
//gsoap tr2 service method-action: GetStreamUri http://www.onvif.org/ver20/media/wsdl/GetStreamUri
int __tr2__GetStreamUri(struct _tr2__GetStreamUri* tr2__GetStreamUri, struct _tr2__GetStreamUriResponse& tr2__GetStreamUriResponse);
//gsoap tr2 service method-action: GetServiceCapabilities http://www.onvif.org/ver20/media/wsdl/GetServiceCapabilities
int __tr2__GetServiceCapabilities(struct _tr2__GetServiceCapabilities* tr2__GetServiceCapabilities, struct _tr2__GetServiceCapabilitiesResponse& tr2__GetServiceCapabilitiesResponse);

//gsoap tptz service name: PTZBinding
//gsoap tptz service namespace: http://www.onvif.org/ver20/ptz/wsdl
//gsoap tptz service style: document
//gsoap tptz service encoding: literal
//gsoap tptz service method-action: GetServiceCapabilities http://www.onvif.org/ver20/ptz/wsdl/GetServiceCapabilities
int __tptz__GetServiceCapabilities(struct _tptz__GetServiceCapabilities* tptz__GetServiceCapabilities, struct _tptz__GetServiceCapabilitiesResponse& tptz__GetServiceCapabilitiesResponse);
//gsoap tptz service method-action: GetNodes http://www.onvif.org/ver20/ptz/wsdl/GetNodes
int __tptz__GetNodes(struct _tptz__GetNodes* tptz__GetNodes, struct _tptz__GetNodesResponse& tptz__GetNodesResponse);
//gsoap tptz service method-action: GetNode http://www.onvif.org/ver20/ptz/wsdl/GetNode
int __tptz__GetNode(struct _tptz__GetNode* tptz__GetNode, struct _tptz__GetNodeResponse& tptz__GetNodeResponse);
//gsoap tptz service method-action: GetConfigurations http://www.onvif.org/ver20/ptz/wsdl/GetConfigurations
int __tptz__GetConfigurations(struct _tptz__GetConfigurations* tptz__GetConfigurations, struct _tptz__GetConfigurationsResponse& tptz__GetConfigurationsResponse);
//gsoap tptz service method-action: GetConfiguration http://www.onvif.org/ver20/ptz/wsdl/GetConfiguration
int __tptz__GetConfiguration(struct _tptz__GetConfiguration* tptz__GetConfiguration, struct _tptz__GetConfigurationResponse& tptz__GetConfigurationResponse);
//gsoap tptz service method-action: GetConfigurationOptions http://www.onvif.org/ver20/ptz/wsdl/GetConfigurationOptions
int __tptz__GetConfigurationOptions(struct _tptz__GetConfigurationOptions* tptz__GetConfigurationOptions, struct _tptz__GetConfigurationOptionsResponse& tptz__GetConfigurationOptionsResponse);
//gsoap tptz service method-action: GetPresets http://www.onvif.org/ver20/ptz/wsdl/GetPresets
int __tptz__GetPresets(struct _tptz__GetPresets* tptz__GetPresets, struct _tptz__GetPresetsResponse& tptz__GetPresetsResponse);
//gsoap tptz service method-action: SetPreset http://www.onvif.org/ver20/ptz/wsdl/SetPreset
int __tptz__SetPreset(struct _tptz__SetPreset* tptz__SetPreset, struct _tptz__SetPresetResponse& tptz__SetPresetResponse);
//gsoap tptz service method-action: GotoPreset http://www.onvif.org/ver20/ptz/wsdl/GotoPreset
int __tptz__GotoPreset(struct _tptz__GotoPreset* tptz__GotoPreset, struct _tptz__GotoPresetResponse& tptz__GotoPresetResponse);
//gsoap tptz service method-action: GetStatus http://www.onvif.org/ver20/ptz/wsdl/GetStatus
int __tptz__GetStatus(struct _tptz__GetStatus* tptz__GetStatus, struct _tptz__GetStatusResponse& tptz__GetStatusResponse);
//gsoap tptz service method-action: GotoHomePosition http://www.onvif.org/ver20/ptz/wsdl/GotoHomePosition
int __tptz__GotoHomePosition(struct _tptz__GotoHomePosition* tptz__GotoHomePosition, struct _tptz__GotoHomePositionResponse& tptz__GotoHomePositionResponse);
//gsoap tptz service method-action: SetHomePosition http://www.onvif.org/ver20/ptz/wsdl/SetHomePosition
int __tptz__SetHomePosition(struct _tptz__SetHomePosition* tptz__SetHomePosition, struct _tptz__SetHomePositionResponse& tptz__SetHomePositionResponse);
//gsoap tptz service method-action: ContinuousMove http://www.onvif.org/ver20/ptz/wsdl/ContinuousMove
int __tptz__ContinuousMove(struct _tptz__ContinuousMove* tptz__ContinuousMove, struct _tptz__ContinuousMoveResponse& tptz__ContinuousMoveResponse);
//gsoap tptz service method-action: RelativeMove http://www.onvif.org/ver20/ptz/wsdl/RelativeMove
int __tptz__RelativeMove(struct _tptz__RelativeMove* tptz__RelativeMove, struct _tptz__RelativeMoveResponse& tptz__RelativeMoveResponse);
//gsoap tptz service method-action: Stop http://www.onvif.org/ver20/ptz/wsdl/Stop
int __tptz__Stop(struct _tptz__Stop* tptz__Stop, struct _tptz__StopResponse& tptz__StopResponse);

//gsoap timg service name: ImagingBinding
//gsoap timg service namespace: http://www.onvif.org/ver20/imaging/wsdl
//gsoap timg service style: document
//gsoap timg service encoding: literal
//gsoap timg service method-action: GetServiceCapabilities http://www.onvif.org/ver20/imaging/wsdl/GetServiceCapabilities
int __timg__GetServiceCapabilities(struct _timg__GetServiceCapabilities* timg__GetServiceCapabilities, struct _timg__GetServiceCapabilitiesResponse& timg__GetServiceCapabilitiesResponse);
//gsoap timg service method-action: GetImagingSettings http://www.onvif.org/ver20/imaging/wsdl/GetImagingSettings
int __timg__GetImagingSettings(struct _timg__GetImagingSettings* timg__GetImagingSettings, struct _timg__GetImagingSettingsResponse& timg__GetImagingSettingsResponse);
//gsoap timg service method-action: SetImagingSettings http://www.onvif.org/ver20/imaging/wsdl/SetImagingSettings
int __timg__SetImagingSettings(struct _timg__SetImagingSettings* timg__SetImagingSettings, struct _timg__SetImagingSettingsResponse& timg__SetImagingSettingsResponse);
//gsoap timg service method-action: GetOptions http://www.onvif.org/ver20/imaging/wsdl/GetOptions
int __timg__GetOptions(struct _timg__GetOptions* timg__GetOptions, struct _timg__GetOptionsResponse& timg__GetOptionsResponse);
