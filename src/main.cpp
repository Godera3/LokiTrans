#include "net/winsock_init.h"
#include "transfer/receiver.h"

#include <cstdint>
#include <iostream>
#include <string>

static void usage() {
    std::cout <<
        "svanipp v1\n"
        "Usage:\n"
        "  svanipp receive --port <p> --out <dir>\n";
}

static bool argEq(const char* a, const char* b) {
    return std::string(a) == std::string(b);
}

int main(int argc, char** argv) {
    WinsockInit wsa;
    if (!wsa.ok()) {
        std::cerr << "WSAStartup failed\n";
        return 1;
    }

    if (argc < 2) { usage(); return 1; }

    std::string cmd = argv[1];
    if (cmd == "receive") {
        uint16_t port = 39000;
        std::string outDir = "Downloads";

        for (int i = 2; i < argc; ++i) {
            if (argEq(argv[i], "--port") && i + 1 < argc) {
                port = static_cast<uint16_t>(std::stoi(argv[++i]));
            } else if (argEq(argv[i], "--out") && i + 1 < argc) {
                outDir = argv[++i];
            } else {
                usage();
                return 1;
            }
        }
        return svanipp::run_receiver(port, outDir);
    }

    usage();
    return 1;
}