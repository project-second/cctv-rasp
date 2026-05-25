#pragma once

#include "config.h"
#include "types.h"

#include <string>

namespace afterveda_onvif {

HttpResponse handle_soap(const Config& config, const std::string& request);

}  // namespace afterveda_onvif
