#ifndef LOKITRANS_CONFIG_HPP
#define LOKITRANS_CONFIG_HPP

#include <string>

namespace lokitrans {

// Discovery constants
constexpr int DISCOVERY_PORT = 48999;
constexpr const char* DISCOVERY_MESSAGE_FORMAT = "LOKITRANS_HELLO|%s|%s";

// Device name handling
std::string get_device_name();

} // namespace lokitrans

#endif // LOKITRANS_CONFIG_HPP