#pragma once
#include <cstdint>
#include <string>

namespace svanipp {
    int run_receiver(std::uint16_t port, const std::string& outDir);
}