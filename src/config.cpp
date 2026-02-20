#include "lokitrans/config.hpp"
#include <fstream>
#include <iostream>
#include <unistd.h>
#include <cstring>

namespace lokitrans {

std::string get_device_name() {
    // Try to read from device_name.txt in the repository root
    std::ifstream file("device_name.txt");
    if (file.is_open()) {
        std::string name;
        std::getline(file, name);
        file.close();
        if (!name.empty()) {
            return name;
        }
    }

    // Fallback to system hostname
    char hostname[256]; // 256 is safe for hostnames
    if (gethostname(hostname, sizeof(hostname)) == 0) {
        hostname[sizeof(hostname) - 1] = '\0'; // ensure null termination
        return std::string(hostname);
    }

    // Ultimate fallback
    return "unknown";
}

} // namespace lokitrans