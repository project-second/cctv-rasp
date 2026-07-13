#pragma once

#include "config.h"

#include "soapH.h"

#include <new>
#include <string>

namespace afterveda_onvif {

Config& current_config(soap* ctx);
int require_auth(soap* ctx);
int invalid_token_fault(soap* ctx, const std::string& token);

template <typename T>
T* soap_make(soap* ctx) {
    void* memory = soap_malloc(ctx, sizeof(T));
    if (!memory) {
        return nullptr;
    }
    return new (memory) T();
}

}  // namespace afterveda_onvif
