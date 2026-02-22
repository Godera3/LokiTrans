#include "discovery/discovery.h"

#define WIN32_LEAN_AND_MEAN
#include <winsock2.h>
#include <ws2tcpip.h>

#include <iostream>
#include <thread>
#include <atomic>
#include <unordered_set>

using namespace std;

namespace {
static constexpr unsigned short DISCOVERY_PORT = 38999;
static constexpr const char* DISCOVER_MSG = "SVANIPP_DISCOVER_V1";
static constexpr const char* HERE_PREFIX  = "SVANIPP_HERE_V1;";
}

void svanipp::discovery::run_responder(uint16_t tcpPort, const string& deviceName) {
    // Runs in background thread; bind UDP 38999 and reply to discovers.
    SOCKET s = ::socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (s == INVALID_SOCKET) return;

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    addr.sin_port = htons(DISCOVERY_PORT);

    if (::bind(s, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) == SOCKET_ERROR) {
        closesocket(s);
        return;
    }

    char buf[512];
    for (;;) {
        sockaddr_in from{};
        int fromLen = sizeof(from);
        int n = ::recvfrom(s, buf, sizeof(buf) - 1, 0, reinterpret_cast<sockaddr*>(&from), &fromLen);
        if (n <= 0) continue;
        buf[n] = '\0';

        if (string(buf) == DISCOVER_MSG) {
            string reply = string(HERE_PREFIX) + deviceName + ";" + to_string(tcpPort);
            ::sendto(s, reply.c_str(), static_cast<int>(reply.size()), 0,
                     reinterpret_cast<sockaddr*>(&from), fromLen);
        }
    }
}

vector<svanipp::discovery::FoundDevice> svanipp::discovery::run_scan(int timeoutMs) {
    vector<FoundDevice> out;

    SOCKET s = ::socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (s == INVALID_SOCKET) return out;

    // Allow broadcast
    BOOL yes = TRUE;
    setsockopt(s, SOL_SOCKET, SO_BROADCAST, reinterpret_cast<const char*>(&yes), sizeof(yes));

    // Bind to ephemeral port for receiving replies
    sockaddr_in bindAddr{};
    bindAddr.sin_family = AF_INET;
    bindAddr.sin_addr.s_addr = htonl(INADDR_ANY);
    bindAddr.sin_port = 0;
    if (::bind(s, reinterpret_cast<sockaddr*>(&bindAddr), sizeof(bindAddr)) == SOCKET_ERROR) {
        closesocket(s);
        return out;
    }

    sockaddr_in bcast{};
    bcast.sin_family = AF_INET;
    bcast.sin_port = htons(DISCOVERY_PORT);
    bcast.sin_addr.s_addr = INADDR_BROADCAST; // 255.255.255.255

    ::sendto(s, DISCOVER_MSG, static_cast<int>(strlen(DISCOVER_MSG)), 0,
             reinterpret_cast<sockaddr*>(&bcast), sizeof(bcast));

    // Receive replies until timeout
    DWORD start = GetTickCount();
    unordered_set<string> seen;

    for (;;) {
        DWORD now = GetTickCount();
        if (static_cast<int>(now - start) >= timeoutMs) break;

        // poll with short timeout
        fd_set fds;
        FD_ZERO(&fds);
        FD_SET(s, &fds);

        timeval tv{};
        tv.tv_sec = 0;
        tv.tv_usec = 150 * 1000;

        int r = ::select(0, &fds, nullptr, nullptr, &tv);
        if (r <= 0) continue;

        char buf[512];
        sockaddr_in from{};
        int fromLen = sizeof(from);
        int n = ::recvfrom(s, buf, sizeof(buf) - 1, 0, reinterpret_cast<sockaddr*>(&from), &fromLen);
        if (n <= 0) continue;
        buf[n] = '\0';

        string msg(buf);
        if (msg.rfind(HERE_PREFIX, 0) == 0) {
            // parse: SVANIPP_HERE_V1;<name>;<port>
            string rest = msg.substr(strlen(HERE_PREFIX));
            auto sep = rest.rfind(';');
            if (sep == string::npos) continue;

            string name = rest.substr(0, sep);
            uint16_t port = static_cast<uint16_t>(stoi(rest.substr(sep + 1)));

            char ipstr[INET_ADDRSTRLEN]{};
            inet_ntop(AF_INET, &from.sin_addr, ipstr, sizeof(ipstr));

            string key = string(ipstr) + ":" + to_string(port);
            if (seen.insert(key).second) {
                out.push_back({ipstr, name, port});
            }
        }
    }

    closesocket(s);
    return out;
}