#ifndef LOKITRANS_NET_HPP
#define LOKITRANS_NET_HPP

#include <vector>
#include <string>

namespace lokitrans {

// Returns a list of non‑loopback IPv4 addresses of the local machine
std::vector<std::string> get_local_ips();

} // namespace lokitrans

#endif // LOKITRANS_NET_HPP