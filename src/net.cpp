#include "lokitrans/net.hpp"
#include <ifaddrs.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <cstring>
#include <iostream>

using namespace std;

namespace lokitrans {

vector<string> get_local_ips() {
    vector<string> ips;
    struct ifaddrs *ifaddr, *ifa;

    if (getifaddrs(&ifaddr) == -1) {
        return ips; // empty on error
    }

    for (ifa = ifaddr; ifa != nullptr; ifa = ifa->ifa_next) {
        if (ifa->ifa_addr == nullptr) continue;

        // Only IPv4 addresses
        if (ifa->ifa_addr->sa_family == AF_INET) {
            struct sockaddr_in *addr = reinterpret_cast<struct sockaddr_in*>(ifa->ifa_addr);
            char ip[INET_ADDRSTRLEN];
            inet_ntop(AF_INET, &addr->sin_addr, ip, sizeof(ip));

            // Skip loopback (127.0.0.0/8)
            if ((addr->sin_addr.s_addr & htonl(0xff000000)) != htonl(0x7f000000)) {
                ips.push_back(ip);
            }
        }
    }

    freeifaddrs(ifaddr);
    return ips;
}

} // namespace lokitrans