#pragma once

#include "config.h"

#include <atomic>

namespace afterveda_onvif {

void http_server(Config config, std::atomic<bool>& running);

}  // namespace afterveda_onvif
