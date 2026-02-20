#pragma once

#include <string>
#include <vector>

struct Peer {
    std::string name;
    std::string ip;
};

void start_listener(int port, const std::string& self_name, const std::string& self_ip);

void broadcast_hello(int port, const std::string& self_name, const std::string& self_ip);

std::vector<Peer> discover_peers(int timeout_ms);