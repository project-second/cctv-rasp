#include "ptz_service.h"

#include "ptz_device.h"
#include "utils.h"

#include <iomanip>
#include <sstream>

namespace afterveda_onvif {
namespace {

std::string ptz_fault(const std::string& error) {
    return error.empty() ? std::string{} : soap_fault(error);
}

std::string onvif_position_value(double value) {
    std::ostringstream out;
    out << std::fixed << std::setprecision(3) << value;
    return out.str();
}

std::string extract_preset_token(const std::string& request) {
    return first_nonempty({
        xml_text_for_local_name(request, "PresetToken"),
        xml_attribute_for_local_name(request, "Preset", "token"),
    });
}

bool is_supported_preset_token(const std::string& token) {
    return token.empty() || token == "home";
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
        const std::string token = extract_preset_token(request);
        if (!is_supported_preset_token(token)) {
            return soap_fault("Invalid or unsupported token: " + token);
        }
        const std::string fault = ptz_fault(set_home_ptz_from_current(config));
        if (!fault.empty()) {
            return fault;
        }
        return soap_envelope("<tptz:SetPresetResponse><tptz:PresetToken>home</tptz:PresetToken></tptz:SetPresetResponse>");
    }
    if (contains(request, "GotoPreset")) {
        const std::string token = extract_preset_token(request);
        if (!is_supported_preset_token(token)) {
            return soap_fault("Invalid or unsupported token: " + token);
        }
        const std::string fault = ptz_fault(home_ptz(config));
        return fault.empty() ? soap_envelope("<tptz:GotoPresetResponse/>") : fault;
    }

    if (contains(request, "GetStatus")) {
        PtzPosition position{};
        const std::string fault = ptz_fault(read_ptz_position(config, position));
        if (!fault.empty()) {
            return fault;
        }
        return soap_envelope(
            "<tptz:GetStatusResponse><tptz:PTZStatus>"
            "<tt:Position><tt:PanTilt x=\"" + onvif_position_value(ptz_pan_to_onvif(position.pan)) +
            "\" y=\"" + onvif_position_value(ptz_tilt_to_onvif(position.tilt)) + "\"/><tt:Zoom x=\"0\"/></tt:Position>"
            "<tt:MoveStatus><tt:PanTilt>IDLE</tt:PanTilt><tt:Zoom>IDLE</tt:Zoom></tt:MoveStatus>"
            "<tt:UtcTime>" + now_utc() + "</tt:UtcTime>"
            "</tptz:PTZStatus></tptz:GetStatusResponse>");
    }

    if (contains(request, "SetHomePosition")) {
        const std::string fault = ptz_fault(set_home_ptz_from_current(config));
        return fault.empty() ? soap_envelope("<tptz:SetHomePositionResponse/>") : fault;
    }
    if (contains(request, "GotoHomePosition")) {
        const std::string fault = ptz_fault(home_ptz(config));
        return fault.empty() ? soap_envelope("<tptz:GotoHomePositionResponse/>") : fault;
    }
    if (contains(request, "Stop")) {
        const std::string fault = ptz_fault(stop_ptz(config));
        return fault.empty() ? soap_envelope("<tptz:StopResponse/>") : fault;
    }

    const std::string fault = ptz_fault(move_ptz_from_request(config, request));
    if (!fault.empty()) {
        return fault;
    }
    if (contains(request, "RelativeMove")) {
        return soap_envelope("<tptz:RelativeMoveResponse/>");
    }
    return soap_envelope("<tptz:ContinuousMoveResponse/>");
}

}  // namespace afterveda_onvif
