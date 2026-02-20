#include "lokitrans/config.hpp"
#include "lokitrans/net.hpp"
#include <iostream>
#include <string>
#include <vector>

using namespace std;

int main(int argc, char* argv[]) {
    if (argc < 2) {
        cerr << "Usage: " << argv[0] << " <command>\n";
        cerr << "Commands:\n";
        cerr << "  whoami    - show device name and local IP addresses\n";
        cerr << "  peers     - discover other devices on the network (not yet implemented)\n";
        return 1;
    }

    string command = argv[1];

    if (command == "whoami") {
        string deviceName = lokitrans::get_device_name();
        vector<string> ips = lokitrans::get_local_ips();

        cout << "DeviceName: " << deviceName << "\n";
        cout << "IPs:\n";
        if (ips.empty()) {
            cout << "  (none)\n";
        } else {
            for (const auto& ip : ips) {
                cout << "  " << ip << "\n";
            }
        }
    }
    else if (command == "peers") {
        cout << "Peer discovery not yet implemented on this platform.\n";
        // In the final project this will call Ankur's discovery module
    }
    else {
        cerr << "Unknown command: " << command << "\n";
        return 1;
    }

    return 0;
}