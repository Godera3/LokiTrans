#include "net/winsock_init.h"
#include "transfer/receiver.h"
#include "transfer/sender.h"
#include "discovery/discovery.h"

#include <cstdlib>
#include <thread>
#include <cstdint>
#include <iostream>
#include <string>

using namespace std;

static void usage() {
    cout <<
        "svanipp v1\n"
        "Usage:\n"
        "  svanipp receive --port <p> --out <dir> [--overwrite]\n"
        "  svanipp send --ip <ip> <file>\n";
        "  svanipp discover\n";
}

static bool argEq(const char* a, const char* b) {
    return string(a) == string(b);
}

int main(int argc, char** argv) {
    WinsockInit wsa;
    if (!wsa.ok()) {
        cerr << "WSAStartup failed\n";
        return 1;
    }

    if (argc < 2) { usage(); return 1; }

    string cmd = argv[1];

    // ===== RECEIVE =====
    if (cmd == "receive") {
        uint16_t port = 39000;
        string outDir = "Downloads";
        bool overwrite = false;

        for (int i = 2; i < argc; ++i) {
            if (argEq(argv[i], "--port") && i + 1 < argc) {
                port = static_cast<uint16_t>(stoi(argv[++i]));
            } else if (argEq(argv[i], "--out") && i + 1 < argc) {
                outDir = argv[++i];
            } else if (argEq(argv[i], "--overwrite")) {
                overwrite = true;
            } else {
                usage();
                return 1;
            }
        }

        string devName = "SvanippDevice";
        if (const char* cn = getenv("COMPUTERNAME")) devName = cn;

        thread([port, devName] {
            svanipp::discovery::run_responder(port, devName);
        }).detach();


        return svanipp::run_receiver(port, outDir, overwrite);
    }

    // ===== SEND =====
    if (cmd == "send") {
        string ip;
        string name;
        string file;
        uint16_t port = 39000;

        for (int i = 2; i < argc; ++i) {
            if (argEq(argv[i], "--ip") && i + 1 < argc) {
                ip = argv[++i];
            } else if (argEq(argv[i], "--name") && i + 1 < argc) {
                name = argv[++i];
            } else {
                file = argv[i];
            }
        }

        if (file.empty() || (ip.empty() && name.empty()) || (!ip.empty() && !name.empty())) {
            usage();
            return 1;
        }

        if (!name.empty()) {
            auto devices = svanipp::discovery::run_scan(1500);

            bool found = false;
            for (const auto& d : devices) {
                if (d.name == name) {
                    ip = d.ip;
                    port = d.port;
                    found = true;
                    break;
                }
            }

            if (!found) {
                cerr << "Device not found: " << name << "\n";
                cerr << "Tip: run `svanipp discover` to see available devices.\n";
                return 1;
            }
        }

        return svanipp::run_sender(ip, port, file);
    }

    // ===== DISCOVER =====
    if (cmd == "discover") {
        auto devices = svanipp::discovery::run_scan(1500);
        if (devices.empty()) {
            cout << "No Svanipp devices found.\n";
            return 0;
        }
        for (const auto& d : devices) {
            cout << d.ip << "  " << d.name << "  " << d.port << "\n";
        }
        return 0;
    }

    usage();
    return 1;
}