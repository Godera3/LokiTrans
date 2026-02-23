#pragma once
#include <cstdint>
#include <string>
#include <vector>

using namespace std;

namespace svanipp::discovery {

static constexpr uint16_t kDiscoveryPort = 38999;

struct FoundDevice {
    string ip;
    string name;
    uint16_t port;
};

void run_responder(uint16_t tcpPort, const string& deviceName);
vector<FoundDevice> run_scan(int timeoutMs = 1500);

}