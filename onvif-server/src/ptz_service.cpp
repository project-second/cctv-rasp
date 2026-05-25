#include "ptz_service.h"

#include "utils.h"

#include <cstdlib>

namespace afterveda_onvif {
namespace {

std::string run_ptz_command(const Config& config, const std::string& command) {
    std::string wire = shell_quote(config.hardware_control);
    if (config.ptz_dry_run) {
        wire += " --dry-run";
    }
    wire += " --step " + std::to_string(config.ptz_step);
    wire += " " + command;
    const int status = std::system(wire.c_str());
    if (status != 0) {
        return soap_fault("PTZ command failed: " + command);
    }
    return {};
}

}  // namespace

std::string handle_ptz(const Config& config, const std::string& request) {
    if (contains(request, "Zoom") && !contains(request, "<tt:Zoom x=\"0") && !contains(request, "<Zoom x=\"0")) {
        return soap_fault("Zoom is not supported by this device");
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
