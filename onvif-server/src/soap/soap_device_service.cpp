#include "soap_common.h"

#include "utils.h"

#include <arpa/inet.h>
#include <ifaddrs.h>
#include <net/if.h>
#include <netinet/in.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <unistd.h>

#include <array>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <ctime>
#include <fstream>
#include <iomanip>
#include <map>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

namespace afterveda_onvif {
namespace {

template <typename T>
T* soap_make_array(soap* ctx, std::size_t count) {
    if (count == 0) {
        return nullptr;
    }
    void* memory = soap_malloc(ctx, sizeof(T) * count);
    if (memory == nullptr) {
        return nullptr;
    }
    T* values = static_cast<T*>(memory);
    for (std::size_t i = 0; i < count; ++i) {
        new (&values[i]) T();
    }
    return values;
}

bool* soap_bool(soap* ctx, bool value) {
    void* memory = soap_malloc(ctx, sizeof(bool));
    if (memory == nullptr) {
        return nullptr;
    }
    auto* out = static_cast<bool*>(memory);
    *out = value;
    return out;
}

void fill_device_information_response(soap* ctx, _tds__GetDeviceInformationResponse& response, const Config& c) {
    response.Manufacturer = soap_strdup(ctx, c.manufacturer.c_str());
    response.Model = soap_strdup(ctx, c.model.c_str());
    response.FirmwareVersion = soap_strdup(ctx, c.firmware.c_str());
    response.SerialNumber = soap_strdup(ctx, c.serial.c_str());
    response.HardwareId = soap_strdup(ctx, c.hardware_id.c_str());
}

int fill_service(soap* ctx, tds__Service& service, const char* ns, const std::string& xaddr, int major, int minor) {
    service.Namespace = soap_strdup(ctx, ns);
    service.XAddr = soap_strdup(ctx, xaddr.c_str());
    service.Version = soap_make<tt__OnvifVersion>(ctx);
    if (service.Namespace == nullptr || service.XAddr == nullptr || service.Version == nullptr) {
        return SOAP_EOM;
    }
    service.Version->Major = major;
    service.Version->Minor = minor;
    return SOAP_OK;
}

int fill_services_response(soap* ctx, _tds__GetServicesResponse& response, const Config& config) {
    struct ServiceSpec {
        const char* ns;
        const char* path;
        int major;
        int minor;
    };

    static constexpr std::array<ServiceSpec, 5> kServices{{
        {"http://www.onvif.org/ver10/device/wsdl", "/device_service", 2, 20},
        {"http://www.onvif.org/ver10/media/wsdl", "/media_service", 2, 20},
        {"http://www.onvif.org/ver20/media/wsdl", "/media2_service", 2, 20},
        {"http://www.onvif.org/ver20/ptz/wsdl", "/ptz_service", 2, 20},
        {"http://www.onvif.org/ver20/imaging/wsdl", "/imaging_service", 2, 20},
    }};

    const std::string base = xaddr_base(config);
    response.__sizeService = static_cast<int>(kServices.size());
    response.Service = soap_make_array<tds__Service>(ctx, kServices.size());
    if (response.Service == nullptr) {
        return SOAP_EOM;
    }
    for (std::size_t i = 0; i < kServices.size(); ++i) {
        const ServiceSpec& spec = kServices[i];
        if (const int err = fill_service(ctx, response.Service[i], spec.ns, base + spec.path, spec.major, spec.minor); err != SOAP_OK) {
            return err;
        }
    }
    return SOAP_OK;
}

tt__DeviceCapabilities* make_device_capabilities(soap* ctx, const std::string& xaddr) {
    tt__DeviceCapabilities* capabilities = soap_make<tt__DeviceCapabilities>(ctx);
    if (capabilities == nullptr) {
        return nullptr;
    }
    capabilities->XAddr = soap_strdup(ctx, xaddr.c_str());
    return capabilities->XAddr == nullptr ? nullptr : capabilities;
}

tt__StreamingCapabilities* make_streaming_capabilities(soap* ctx, bool include_rtp_tcp, bool include_rtsp_tcp) {
    tt__StreamingCapabilities* streaming = soap_make<tt__StreamingCapabilities>(ctx);
    if (streaming == nullptr) {
        return nullptr;
    }
    streaming->RTPMulticast = soap_bool(ctx, false);
    streaming->RTP_USCORETCP = include_rtp_tcp ? soap_bool(ctx, true) : nullptr;
    streaming->RTP_USCORERTSP_USCORETCP = include_rtsp_tcp ? soap_bool(ctx, true) : nullptr;
    if (streaming->RTPMulticast == nullptr ||
        (include_rtp_tcp && streaming->RTP_USCORETCP == nullptr) ||
        (include_rtsp_tcp && streaming->RTP_USCORERTSP_USCORETCP == nullptr)) {
        return nullptr;
    }
    return streaming;
}

tt__MediaCapabilities* make_media_capabilities(soap* ctx, const std::string& xaddr, bool include_rtp_tcp, bool include_rtsp_tcp) {
    tt__MediaCapabilities* capabilities = soap_make<tt__MediaCapabilities>(ctx);
    if (capabilities == nullptr) {
        return nullptr;
    }
    capabilities->XAddr = soap_strdup(ctx, xaddr.c_str());
    capabilities->StreamingCapabilities = make_streaming_capabilities(ctx, include_rtp_tcp, include_rtsp_tcp);
    if (capabilities->XAddr == nullptr || capabilities->StreamingCapabilities == nullptr) {
        return nullptr;
    }
    return capabilities;
}

tt__PTZCapabilities* make_ptz_capabilities(soap* ctx, const std::string& xaddr) {
    tt__PTZCapabilities* capabilities = soap_make<tt__PTZCapabilities>(ctx);
    if (capabilities == nullptr) {
        return nullptr;
    }
    capabilities->XAddr = soap_strdup(ctx, xaddr.c_str());
    return capabilities->XAddr == nullptr ? nullptr : capabilities;
}

tt__ImagingCapabilities* make_imaging_capabilities(soap* ctx, const std::string& xaddr) {
    tt__ImagingCapabilities* capabilities = soap_make<tt__ImagingCapabilities>(ctx);
    if (capabilities == nullptr) {
        return nullptr;
    }
    capabilities->XAddr = soap_strdup(ctx, xaddr.c_str());
    return capabilities->XAddr == nullptr ? nullptr : capabilities;
}

bool category_requested(const _tds__GetCapabilities* request, tds__CapabilityCategory category) {
    if (request == nullptr || request->__sizeCategory == 0 || request->Category == nullptr) {
        return true;
    }
    for (int i = 0; i < request->__sizeCategory; ++i) {
        if (request->Category[i] == tds__CapabilityCategory__All || request->Category[i] == category) {
            return true;
        }
    }
    return false;
}

int fill_capabilities_response(soap* ctx, _tds__GetCapabilitiesResponse& response,
                               const _tds__GetCapabilities* request, const Config& config) {
    const std::string base = xaddr_base(config);
    response.Capabilities = soap_make<tt__Capabilities>(ctx);
    if (response.Capabilities == nullptr) {
        return SOAP_EOM;
    }

    const bool include_device = category_requested(request, tds__CapabilityCategory__Device);
    const bool include_media = category_requested(request, tds__CapabilityCategory__Media);
    const bool include_ptz = category_requested(request, tds__CapabilityCategory__PTZ);
    const bool include_imaging = category_requested(request, tds__CapabilityCategory__Imaging);

    response.Capabilities->Device = include_device ? make_device_capabilities(ctx, base + "/device_service") : nullptr;
    response.Capabilities->Media = include_media ? make_media_capabilities(ctx, base + "/media_service", true, true) : nullptr;
    response.Capabilities->Media2 = include_media ? make_media_capabilities(ctx, base + "/media2_service", false, true) : nullptr;
    response.Capabilities->PTZ = include_ptz ? make_ptz_capabilities(ctx, base + "/ptz_service") : nullptr;
    response.Capabilities->Imaging = include_imaging ? make_imaging_capabilities(ctx, base + "/imaging_service") : nullptr;

    if ((include_device && response.Capabilities->Device == nullptr) ||
        (include_media && (response.Capabilities->Media == nullptr || response.Capabilities->Media2 == nullptr)) ||
        (include_ptz && response.Capabilities->PTZ == nullptr) ||
        (include_imaging && response.Capabilities->Imaging == nullptr)) {
        return SOAP_EOM;
    }
    return SOAP_OK;
}

int fill_service_capabilities_response(soap* ctx, _tds__GetServiceCapabilitiesResponse& response) {
    response.Capabilities = soap_make<tds__DeviceServiceCapabilities>(ctx);
    if (response.Capabilities == nullptr) {
        return SOAP_EOM;
    }

    response.Capabilities->Network = soap_make<tds__NetworkCapabilities>(ctx);
    response.Capabilities->Security = soap_make<tds__SecurityCapabilities>(ctx);
    response.Capabilities->System = soap_make<tds__SystemCapabilities>(ctx);
    if (response.Capabilities->Network == nullptr ||
        response.Capabilities->Security == nullptr ||
        response.Capabilities->System == nullptr) {
        return SOAP_EOM;
    }

    response.Capabilities->Network->IPFilter = false;
    response.Capabilities->Network->ZeroConfiguration = false;
    response.Capabilities->Network->IPVersion6 = false;
    response.Capabilities->Network->DynDNS = false;

    response.Capabilities->Security->OnboardKeyGeneration = false;
    response.Capabilities->Security->AccessPolicyConfig = false;
    response.Capabilities->Security->DefaultAccessPolicy = false;
    response.Capabilities->Security->Dot1X = false;
    response.Capabilities->Security->RemoteUserHandling = false;

    response.Capabilities->System->DiscoveryResolve = true;
    response.Capabilities->System->DiscoveryBye = false;
    response.Capabilities->System->RemoteDiscovery = false;
    response.Capabilities->System->SystemBackup = false;
    response.Capabilities->System->SystemLogging = false;
    response.Capabilities->System->FirmwareUpgrade = false;
    return SOAP_OK;
}

int fill_system_date_and_time_response(soap* ctx, _tds__GetSystemDateAndTimeResponse& response) {
    std::time_t raw = std::time(nullptr);
    std::tm tm{};
    gmtime_r(&raw, &tm);

    response.SystemDateAndTime = soap_make<tt__SystemDateTime>(ctx);
    if (response.SystemDateAndTime == nullptr) {
        return SOAP_EOM;
    }
    response.SystemDateAndTime->DateTimeType = soap_strdup(ctx, "NTP");
    response.SystemDateAndTime->DaylightSavings = false;
    response.SystemDateAndTime->UTCDateTime = soap_make<tt__DateTime>(ctx);
    if (response.SystemDateAndTime->DateTimeType == nullptr ||
        response.SystemDateAndTime->UTCDateTime == nullptr) {
        return SOAP_EOM;
    }

    response.SystemDateAndTime->UTCDateTime->Time = soap_make<tt__Time>(ctx);
    response.SystemDateAndTime->UTCDateTime->Date = soap_make<tt__Date>(ctx);
    if (response.SystemDateAndTime->UTCDateTime->Time == nullptr ||
        response.SystemDateAndTime->UTCDateTime->Date == nullptr) {
        return SOAP_EOM;
    }

    response.SystemDateAndTime->UTCDateTime->Time->Hour = tm.tm_hour;
    response.SystemDateAndTime->UTCDateTime->Time->Minute = tm.tm_min;
    response.SystemDateAndTime->UTCDateTime->Time->Second = tm.tm_sec;
    response.SystemDateAndTime->UTCDateTime->Date->Year = tm.tm_year + 1900;
    response.SystemDateAndTime->UTCDateTime->Date->Month = tm.tm_mon + 1;
    response.SystemDateAndTime->UTCDateTime->Date->Day = tm.tm_mday;
    return SOAP_OK;
}

int fill_scope(soap* ctx, tt__Scope& scope, const std::string& item) {
    scope.ScopeDef = soap_strdup(ctx, "Fixed");
    scope.ScopeItem = soap_strdup(ctx, item.c_str());
    return scope.ScopeDef == nullptr || scope.ScopeItem == nullptr ? SOAP_EOM : SOAP_OK;
}

int fill_scopes_response(soap* ctx, _tds__GetScopesResponse& response, const Config& config) {
    static constexpr std::size_t kScopeCount = 2;
    response.__sizeScopes = static_cast<int>(kScopeCount);
    response.Scopes = soap_make_array<tt__Scope>(ctx, kScopeCount);
    if (response.Scopes == nullptr) {
        return SOAP_EOM;
    }

    const std::array<std::string, kScopeCount> scopes{{
        "onvif://www.onvif.org/name/" + config.device_name,
        "onvif://www.onvif.org/hardware/" + config.hardware_id,
    }};
    for (std::size_t i = 0; i < scopes.size(); ++i) {
        if (const int err = fill_scope(ctx, response.Scopes[i], scopes[i]); err != SOAP_OK) {
            return err;
        }
    }
    return SOAP_OK;
}

int fill_hostname_response(soap* ctx, _tds__GetHostnameResponse& response, const Config& config) {
    response.HostnameInformation = soap_make<tt__HostnameInformation>(ctx);
    if (response.HostnameInformation == nullptr) {
        return SOAP_EOM;
    }
    response.HostnameInformation->FromDHCP = false;
    response.HostnameInformation->Name = soap_strdup(ctx, config.device_name.c_str());
    return response.HostnameInformation->Name == nullptr ? SOAP_EOM : SOAP_OK;
}

struct IPv4AddressSnapshot {
    std::string address;
    int prefix_length = 0;
};

struct NetworkInterfaceSnapshot {
    std::string name;
    std::string hardware_address;
    int mtu = 0;
    bool enabled = false;
    bool dhcp = false;
    std::vector<IPv4AddressSnapshot> ipv4_addresses;
};

bool path_exists(const std::string& path) {
    struct stat info {};
    return stat(path.c_str(), &info) == 0;
}

bool file_contains(const std::string& path, const std::string& needle) {
    std::ifstream input(path);
    if (!input) {
        return false;
    }
    std::string line;
    while (std::getline(input, line)) {
        if (line.find(needle) != std::string::npos) {
            return true;
        }
    }
    return false;
}

bool has_dhcp_lease(const std::string& name) {
    const unsigned int index = if_nametoindex(name.c_str());
    if (index != 0) {
        const std::string index_text = std::to_string(index);
        if (path_exists("/run/systemd/netif/leases/" + index_text) ||
            file_contains("/run/NetworkManager/devices/" + index_text, "DHCP4.OPTION")) {
            return true;
        }
    }
    return path_exists("/var/lib/dhcpcd5/dhcpcd-" + name + ".lease") ||
           path_exists("/var/lib/dhcpcd/" + name + ".lease") ||
           path_exists("/run/dhcpcd/" + name + ".lease");
}

int prefix_length(const sockaddr* netmask) {
    if (netmask == nullptr || netmask->sa_family != AF_INET) {
        return 0;
    }
    const auto* mask = reinterpret_cast<const sockaddr_in*>(netmask);
    std::uint32_t bits = ntohl(mask->sin_addr.s_addr);
    int prefix = 0;
    while ((bits & 0x80000000U) != 0U) {
        ++prefix;
        bits <<= 1U;
    }
    return prefix;
}

void read_link_details(NetworkInterfaceSnapshot& snapshot, int socket_fd) {
    ifreq request {};
    std::strncpy(request.ifr_name, snapshot.name.c_str(), IFNAMSIZ - 1);
    if (ioctl(socket_fd, SIOCGIFMTU, &request) == 0) {
        snapshot.mtu = request.ifr_mtu;
    }

    std::memset(&request, 0, sizeof(request));
    std::strncpy(request.ifr_name, snapshot.name.c_str(), IFNAMSIZ - 1);
    if (ioctl(socket_fd, SIOCGIFHWADDR, &request) == 0) {
        const auto* bytes = reinterpret_cast<const unsigned char*>(request.ifr_hwaddr.sa_data);
        std::ostringstream address;
        address << std::hex << std::setfill('0');
        for (int i = 0; i < 6; ++i) {
            if (i != 0) address << ':';
            address << std::setw(2) << static_cast<unsigned int>(bytes[i]);
        }
        snapshot.hardware_address = address.str();
    }
}

std::vector<NetworkInterfaceSnapshot> read_network_interfaces() {
    ifaddrs* addresses = nullptr;
    if (getifaddrs(&addresses) != 0) {
        return {};
    }

    std::map<std::string, NetworkInterfaceSnapshot> snapshots;
    for (const ifaddrs* current = addresses; current != nullptr; current = current->ifa_next) {
        if (current->ifa_name == nullptr || current->ifa_addr == nullptr ||
            (current->ifa_flags & IFF_LOOPBACK) != 0) {
            continue;
        }
        NetworkInterfaceSnapshot& snapshot = snapshots[current->ifa_name];
        snapshot.name = current->ifa_name;
        snapshot.enabled = snapshot.enabled || (current->ifa_flags & IFF_UP) != 0;

        if (current->ifa_addr->sa_family == AF_INET) {
            const auto* address = reinterpret_cast<const sockaddr_in*>(current->ifa_addr);
            char text[INET_ADDRSTRLEN] {};
            if (inet_ntop(AF_INET, &address->sin_addr, text, sizeof(text)) != nullptr) {
                snapshot.ipv4_addresses.push_back({text, prefix_length(current->ifa_netmask)});
            }
        }
    }
    freeifaddrs(addresses);

    const int socket_fd = socket(AF_INET, SOCK_DGRAM, 0);
    std::vector<NetworkInterfaceSnapshot> result;
    result.reserve(snapshots.size());
    for (auto& entry : snapshots) {
        NetworkInterfaceSnapshot& snapshot = entry.second;
        if (socket_fd >= 0) {
            read_link_details(snapshot, socket_fd);
        }
        snapshot.dhcp = has_dhcp_lease(snapshot.name);
        result.push_back(std::move(snapshot));
    }
    if (socket_fd >= 0) {
        close(socket_fd);
    }
    return result;
}

int fill_prefixed_address(soap* ctx, tt__PrefixedIPv4Address& target, const IPv4AddressSnapshot& source) {
    target.Address = soap_strdup(ctx, source.address.c_str());
    target.PrefixLength = source.prefix_length;
    return target.Address == nullptr ? SOAP_EOM : SOAP_OK;
}

int fill_network_interfaces_response(soap* ctx, _tds__GetNetworkInterfacesResponse& response) {
    const std::vector<NetworkInterfaceSnapshot> snapshots = read_network_interfaces();
    response.__sizeNetworkInterfaces = static_cast<int>(snapshots.size());
    response.NetworkInterfaces = soap_make_array<tt__NetworkInterface>(ctx, snapshots.size());
    if (snapshots.empty()) {
        return SOAP_OK;
    }
    if (response.NetworkInterfaces == nullptr) {
        return SOAP_EOM;
    }

    for (std::size_t i = 0; i < snapshots.size(); ++i) {
        const NetworkInterfaceSnapshot& snapshot = snapshots[i];
        tt__NetworkInterface& iface = response.NetworkInterfaces[i];
        iface.token = soap_strdup(ctx, snapshot.name.c_str());
        iface.enabled = snapshot.enabled;
        iface.Info = soap_make<tt__NetworkInterfaceInfo>(ctx);
        iface.IPv4 = soap_make<tt__IPv4NetworkInterface>(ctx);
        if (iface.token == nullptr || iface.Info == nullptr || iface.IPv4 == nullptr) {
            return SOAP_EOM;
        }

        iface.Info->Name = soap_strdup(ctx, snapshot.name.c_str());
        iface.Info->HwAddress = soap_strdup(ctx, snapshot.hardware_address.c_str());
        iface.Info->MTU = snapshot.mtu;
        iface.IPv4->Enabled = snapshot.enabled;
        iface.IPv4->Config = soap_make<tt__IPv4Configuration>(ctx);
        if (iface.Info->Name == nullptr || iface.Info->HwAddress == nullptr || iface.IPv4->Config == nullptr) {
            return SOAP_EOM;
        }

        tt__IPv4Configuration& config = *iface.IPv4->Config;
        config.DHCP = snapshot.dhcp;
        config.__sizeManual = snapshot.dhcp ? 0 : static_cast<int>(snapshot.ipv4_addresses.size());
        config.Manual = snapshot.dhcp ? nullptr : soap_make_array<tt__PrefixedIPv4Address>(ctx, snapshot.ipv4_addresses.size());
        config.LinkLocal = nullptr;
        config.FromDHCP = nullptr;
        if (snapshot.dhcp && !snapshot.ipv4_addresses.empty()) {
            config.FromDHCP = soap_make<tt__PrefixedIPv4Address>(ctx);
            if (config.FromDHCP == nullptr ||
                fill_prefixed_address(ctx, *config.FromDHCP, snapshot.ipv4_addresses.front()) != SOAP_OK) {
                return SOAP_EOM;
            }
        } else if (!snapshot.ipv4_addresses.empty()) {
            if (config.Manual == nullptr) {
                return SOAP_EOM;
            }
            for (std::size_t address_index = 0; address_index < snapshot.ipv4_addresses.size(); ++address_index) {
                if (fill_prefixed_address(ctx, config.Manual[address_index], snapshot.ipv4_addresses[address_index]) != SOAP_OK) {
                    return SOAP_EOM;
                }
            }
        }
    }
    return SOAP_OK;
}

int fill_users_response(soap* ctx, _tds__GetUsersResponse& response, const Config& config) {
    if (config.username.empty()) {
        response.__sizeUser = 0;
        response.User = nullptr;
        return SOAP_OK;
    }

    response.__sizeUser = 1;
    response.User = soap_make_array<tt__User>(ctx, 1);
    if (response.User == nullptr) {
        return SOAP_EOM;
    }
    response.User[0].Username = soap_strdup(ctx, config.username.c_str());
    response.User[0].UserLevel = soap_strdup(ctx, "Administrator");
    return response.User[0].Username == nullptr || response.User[0].UserLevel == nullptr ? SOAP_EOM : SOAP_OK;
}

}  // namespace
}  // namespace afterveda_onvif

using namespace afterveda_onvif;

int __tds__GetCapabilities(soap* ctx, _tds__GetCapabilities* request, _tds__GetCapabilitiesResponse& response) {
    if (const int err = require_auth(ctx); err != SOAP_OK) return err;
    return fill_capabilities_response(ctx, response, request, current_config(ctx));
}

int __tds__GetServices(soap* ctx, _tds__GetServices*, _tds__GetServicesResponse& response) {
    if (const int err = require_auth(ctx); err != SOAP_OK) return err;
    return fill_services_response(ctx, response, current_config(ctx));
}

int __tds__GetDeviceInformation(soap* ctx, _tds__GetDeviceInformation*, _tds__GetDeviceInformationResponse& response) {
    if (const int err = require_auth(ctx); err != SOAP_OK) return err;
    fill_device_information_response(ctx, response, current_config(ctx));
    return SOAP_OK;
}

int __tds__GetSystemDateAndTime(soap* ctx, _tds__GetSystemDateAndTime*, _tds__GetSystemDateAndTimeResponse& response) {
    if (const int err = require_auth(ctx); err != SOAP_OK) return err;
    return fill_system_date_and_time_response(ctx, response);
}

int __tds__GetScopes(soap* ctx, _tds__GetScopes*, _tds__GetScopesResponse& response) {
    if (const int err = require_auth(ctx); err != SOAP_OK) return err;
    return fill_scopes_response(ctx, response, current_config(ctx));
}

int __tds__GetHostname(soap* ctx, _tds__GetHostname*, _tds__GetHostnameResponse& response) {
    if (const int err = require_auth(ctx); err != SOAP_OK) return err;
    return fill_hostname_response(ctx, response, current_config(ctx));
}

int __tds__GetNetworkInterfaces(soap* ctx, _tds__GetNetworkInterfaces*, _tds__GetNetworkInterfacesResponse& response) {
    if (const int err = require_auth(ctx); err != SOAP_OK) return err;
    return fill_network_interfaces_response(ctx, response);
}

int __tds__GetUsers(soap* ctx, _tds__GetUsers*, _tds__GetUsersResponse& response) {
    if (const int err = require_auth(ctx); err != SOAP_OK) return err;
    return fill_users_response(ctx, response, current_config(ctx));
}

int __tds__GetServiceCapabilities(soap* ctx, _tds__GetServiceCapabilities*, _tds__GetServiceCapabilitiesResponse& response) {
    if (const int err = require_auth(ctx); err != SOAP_OK) return err;
    return fill_service_capabilities_response(ctx, response);
}
