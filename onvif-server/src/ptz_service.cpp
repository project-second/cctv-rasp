#include "ptz_service.h"

#include "ptz_device.h"
#include "utils.h"

namespace afterveda_onvif {
namespace {

std::string run_ptz_command(const Config& config, const std::string& command) {
    const std::string error = write_ptz_command(config, command);
    return error.empty() ? std::string{} : soap_fault(error);
}

}  // namespace

std::string handle_ptz(const Config& config, const std::string& request) {
    if (contains(request, "Zoom") && !contains(request, "<tt:Zoom x=\"0") && !contains(request, "<Zoom x=\"0")) {
        return soap_fault("Zoom is not supported by this device");
    }

    if (contains(request, "GetNodes")) {
        return soap_envelope(
            "<tptz:GetNodesResponse><tptz:PTZNode token=\"ptz-node\">"
            "<tt:Name>PanTilt</tt:Name>"
            "<tt:SupportedPTZSpaces>"
            "<tt:ContinuousPanTiltVelocitySpace><tt:URI>http://www.onvif.org/ver10/tptz/PanTiltSpaces/VelocityGenericSpace</tt:URI><tt:XRange><tt:Min>-1</tt:Min><tt:Max>1</tt:Max></tt:XRange><tt:YRange><tt:Min>-1</tt:Min><tt:Max>1</tt:Max></tt:YRange></tt:ContinuousPanTiltVelocitySpace>"
            "<tt:RelativePanTiltTranslationSpace><tt:URI>http://www.onvif.org/ver10/tptz/PanTiltSpaces/TranslationGenericSpace</tt:URI><tt:XRange><tt:Min>-1</tt:Min><tt:Max>1</tt:Max></tt:XRange><tt:YRange><tt:Min>-1</tt:Min><tt:Max>1</tt:Max></tt:YRange></tt:RelativePanTiltTranslationSpace>"
            "</tt:SupportedPTZSpaces>"
            "<tt:MaximumNumberOfPresets>1</tt:MaximumNumberOfPresets><tt:HomeSupported>true</tt:HomeSupported>"
            "</tptz:PTZNode></tptz:GetNodesResponse>");
    }
    if (contains(request, "GetConfigurations")) {
        return soap_envelope(
            "<tptz:GetConfigurationsResponse><tptz:PTZConfiguration token=\"ptz\">"
            "<tt:Name>PanTilt</tt:Name><tt:UseCount>1</tt:UseCount><tt:NodeToken>ptz-node</tt:NodeToken>"
            "<tt:DefaultContinuousPanTiltVelocitySpace>http://www.onvif.org/ver10/tptz/PanTiltSpaces/VelocityGenericSpace</tt:DefaultContinuousPanTiltVelocitySpace>"
            "<tt:DefaultRelativePanTiltTranslationSpace>http://www.onvif.org/ver10/tptz/PanTiltSpaces/TranslationGenericSpace</tt:DefaultRelativePanTiltTranslationSpace>"
            "<tt:DefaultPTZTimeout>PT1S</tt:DefaultPTZTimeout>"
            "<tt:PanTiltLimits><tt:Range><tt:URI>http://www.onvif.org/ver10/tptz/PanTiltSpaces/PositionGenericSpace</tt:URI><tt:XRange><tt:Min>-1</tt:Min><tt:Max>1</tt:Max></tt:XRange><tt:YRange><tt:Min>-1</tt:Min><tt:Max>1</tt:Max></tt:YRange></tt:Range></tt:PanTiltLimits>"
            "</tptz:PTZConfiguration></tptz:GetConfigurationsResponse>");
    }
    if (contains(request, "GetPresets")) {
        return soap_envelope("<tptz:GetPresetsResponse><tptz:Preset token=\"home\"><tt:Name>Home</tt:Name></tptz:Preset></tptz:GetPresetsResponse>");
    }
    if (contains(request, "SetPreset")) {
        return soap_envelope("<tptz:SetPresetResponse><tptz:PresetToken>home</tptz:PresetToken></tptz:SetPresetResponse>");
    }
    if (contains(request, "GotoPreset")) {
        const std::string fault = run_ptz_command(config, "center");
        return fault.empty() ? soap_envelope("<tptz:GotoPresetResponse/>") : fault;
    }

    if (contains(request, "GetStatus")) {
        return soap_envelope(
            "<tptz:GetStatusResponse><tptz:PTZStatus>"
            "<tt:Position><tt:PanTilt x=\"0\" y=\"0\"/><tt:Zoom x=\"0\"/></tt:Position>"
            "<tt:MoveStatus><tt:PanTilt>IDLE</tt:PanTilt><tt:Zoom>IDLE</tt:Zoom></tt:MoveStatus>"
            "<tt:UtcTime>" + now_utc() + "</tt:UtcTime>"
            "</tptz:PTZStatus></tptz:GetStatusResponse>");
    }

    if (contains(request, "SetHomePosition")) {
        const std::string fault = run_ptz_command(config, "center");
        return fault.empty() ? soap_envelope("<tptz:SetHomePositionResponse/>") : fault;
    }
    if (contains(request, "GotoHomePosition")) {
        const std::string fault = run_ptz_command(config, "center");
        return fault.empty() ? soap_envelope("<tptz:GotoHomePositionResponse/>") : fault;
    }
    if (contains(request, "Stop")) {
        const std::string fault = run_ptz_command(config, "stop");
        return fault.empty() ? soap_envelope("<tptz:StopResponse/>") : fault;
    }

    std::string command = "stop";
    const bool negative_x = contains(request, "x=\"-") || contains(request, "<tt:x>-");
    const bool positive_x = contains(request, "x=\"0.") || contains(request, "x=\"1") || contains(request, "<tt:x>0.") || contains(request, "<tt:x>1");
    const bool negative_y = contains(request, "y=\"-") || contains(request, "<tt:y>-");
    const bool positive_y = contains(request, "y=\"0.") || contains(request, "y=\"1") || contains(request, "<tt:y>0.") || contains(request, "<tt:y>1");
    if (negative_x) {
        command = "left";
    } else if (positive_x) {
        command = "right";
    } else if (negative_y) {
        command = "up";
    } else if (positive_y) {
        command = "down";
    }

    const std::string fault = run_ptz_command(config, command);
    if (!fault.empty()) {
        return fault;
    }
    if (contains(request, "RelativeMove")) {
        return soap_envelope("<tptz:RelativeMoveResponse/>");
    }
    return soap_envelope("<tptz:ContinuousMoveResponse/>");
}

}  // namespace afterveda_onvif
