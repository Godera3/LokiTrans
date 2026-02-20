#include "lokitrans/discovery.hpp"

#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>

#include <iostream>
#include <thread>
#include <mutex>
#include <atomic>

#include <chrono>
#include <vector>

#pragma comment(lib, "Ws2_32.lib")

static std::vector<Peer> g_peers;
static std::mutex g_peers_mutex;
static std::atomic<bool> g_running = false;

static bool init_winsock() {
    static bool initialized = false;
    if (initialized) return true;

    WSADATA wsaData;
    int result = WSAStartup(MAKEWORD(2,2), &wsaData);
    if (result != 0) {
        std::cerr << "WSAStartup failed: " << result << "\n";
        return false;
    }

    initialized = true;
    return true;
}

void start_listener(int port, const std::string& self_name, const std::string& self_ip)
{
    if (!init_winsock()) return;

    g_running = true;

    std::thread([port, self_name, self_ip]() {

        SOCKET sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
        if (sock == INVALID_SOCKET) {
            std::cerr << "Socket creation failed\n";
            return;
        }

        sockaddr_in addr{};
        addr.sin_family = AF_INET;
        addr.sin_addr.s_addr = INADDR_ANY;
        addr.sin_port = htons(port);

        if (bind(sock, (sockaddr*)&addr, sizeof(addr)) == SOCKET_ERROR) {
            std::cerr << "Bind failed\n";
            closesocket(sock);
            return;
        }

        char buffer[1024];

        while (g_running) {
            sockaddr_in sender{};
            int sender_len = sizeof(sender);

            int bytes = recvfrom(sock, buffer, sizeof(buffer) - 1, 0,
                                 (sockaddr*)&sender, &sender_len);

            if (bytes <= 0) continue;

            buffer[bytes] = '\0';
            std::string message(buffer);

            if (message.rfind("LOKITRANS_HELLO", 0) == 0) {

                // Format: LOKITRANS_HELLO|name|ip
                size_t first = message.find('|');
                size_t second = message.find('|', first + 1);

                if (first != std::string::npos && second != std::string::npos) {
                    std::string name = message.substr(first + 1, second - first - 1);
                    std::string ip = message.substr(second + 1);
                    
                    if (ip == self_ip) {
                        continue; // Ignore our own hello
                    }
                    
                    {
                        std::lock_guard<std::mutex> lock(g_peers_mutex);

                        bool exists = false;
                        for (auto& p : g_peers) {
                            if (p.ip == ip) {
                                exists = true;
                                break;
                            }
                        }

                        if (!exists) {
                            g_peers.push_back({ name, ip });
                            std::cout << "Discovered peer: " << name << " (" << ip << ")\n";
                        }
                    }

                    // Send back our hello (unicast)
                    std::string response = "LOKITRANS_HELLO|" + self_name + "|" + self_ip;

                    sendto(sock,
                           response.c_str(),
                           (int)response.size(),
                           0,
                           (sockaddr*)&sender,
                           sender_len);
                }
            }
        }

        closesocket(sock);

    }).detach();
}

void broadcast_hello(int port, const std::string& self_name, const std::string& self_ip)
{
    if (!init_winsock()) return;

    SOCKET sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (sock == INVALID_SOCKET) {
        std::cerr << "Broadcast socket failed\n";
        return;
    }

    BOOL broadcastEnable = TRUE;
    setsockopt(sock, SOL_SOCKET, SO_BROADCAST,
               (char*)&broadcastEnable,
               sizeof(broadcastEnable));

    sockaddr_in broadcastAddr{};
    broadcastAddr.sin_family = AF_INET;
    broadcastAddr.sin_port = htons(port);
    broadcastAddr.sin_addr.s_addr = INADDR_BROADCAST; // 255.255.255.255

    std::string message = "LOKITRANS_HELLO|" + self_name + "|" + self_ip;

    sendto(sock,
           message.c_str(),
           (int)message.size(),
           0,
           (sockaddr*)&broadcastAddr,
           sizeof(broadcastAddr));

    closesocket(sock);
}

std::vector<Peer> discover_peers(int timeout_ms)
{
    {
        std::lock_guard<std::mutex> lock(g_peers_mutex);
        g_peers.clear();
    }

    // broadcast_hello(48999, "Ankur", "192.168.1.10"); // pass real values in production
    std::vector<Peer> discover_peers(
    int port,
    const std::string& self_name,
    const std::string& self_ip,
    int timeout_ms
);

    std::this_thread::sleep_for(std::chrono::milliseconds(timeout_ms));

    std::lock_guard<std::mutex> lock(g_peers_mutex);
    return g_peers;
}