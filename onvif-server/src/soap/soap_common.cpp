#include "soap_common.h"

#include "utils.h"

#include "DeviceBinding.nsmap"

#include <vector>

namespace afterveda_onvif {

Config& current_config(soap* ctx) {
    return *static_cast<Config*>(ctx->user);
}

int require_auth(soap* ctx) {
    const Config& config = current_config(ctx);
    if (config.username.empty() && config.password.empty()) {
        return SOAP_OK;
    }
    if (!ctx->header || !ctx->header->wsse__Security || !ctx->header->wsse__Security->UsernameToken) {
        return soap_sender_fault(ctx, "ONVIF authentication required", nullptr);
    }
    const _wsse__UsernameToken* token = ctx->header->wsse__Security->UsernameToken;
    if (!token->Username || config.username != token->Username || !token->Password || !token->Password->__item) {
        return soap_sender_fault(ctx, "ONVIF authentication failed", nullptr);
    }
    const std::string password = token->Password->__item;
    const std::string type = token->Password->Type ? token->Password->Type : "";
    if (contains(type, "PasswordDigest")) {
        if (!token->Nonce || !token->Nonce->__item || !token->wsu__Created || !utc_timestamp_within(token->wsu__Created, 300)) {
            return soap_sender_fault(ctx, "ONVIF authentication failed", nullptr);
        }
        std::vector<unsigned char> digest_input = base64_decode(token->Nonce->__item);
        if (digest_input.empty()) {
            return soap_sender_fault(ctx, "ONVIF authentication failed", nullptr);
        }
        digest_input.insert(digest_input.end(), token->wsu__Created, token->wsu__Created + std::char_traits<char>::length(token->wsu__Created));
        digest_input.insert(digest_input.end(), config.password.begin(), config.password.end());
        if (base64_encode(sha1(digest_input)) != password) {
            return soap_sender_fault(ctx, "ONVIF authentication failed", nullptr);
        }
        return SOAP_OK;
    }
    if (password != config.password) {
        return soap_sender_fault(ctx, "ONVIF authentication failed", nullptr);
    }
    return SOAP_OK;
}

int invalid_token_fault(soap* ctx, const std::string& token) {
    return soap_sender_fault(ctx, ("Invalid or unsupported token: " + token).c_str(), nullptr);
}

}  // namespace afterveda_onvif
