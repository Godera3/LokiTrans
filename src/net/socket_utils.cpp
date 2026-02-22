#include "net/socket_utils.h"

#ifdef _WIN32
#include <winsock2.h>
#else
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#endif

static uint64_t bswap64_local(uint64_t x) {
    return ((x & 0x00000000000000FFULL) << 56) |
           ((x & 0x000000000000FF00ULL) << 40) |
           ((x & 0x0000000000FF0000ULL) << 24) |
           ((x & 0x00000000FF000000ULL) << 8)  |
           ((x & 0x000000FF00000000ULL) >> 8)  |
           ((x & 0x0000FF0000000000ULL) >> 24) |
           ((x & 0x00FF000000000000ULL) >> 40) |
           ((x & 0xFF00000000000000ULL) >> 56);
}

uint16_t ntoh_u16(uint16_t v) { return ntohs(v); }
uint16_t hton_u16(uint16_t v) { return htons(v); }

uint64_t ntoh_u64(uint64_t v) {
#if defined(_WIN32)
    // Windows is little-endian on typical targets; network is big-endian.
    return bswap64_local(v);
#else
#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
    return bswap64_local(v);
#else
    return v;
#endif
#endif
}

uint64_t hton_u64(uint64_t v) { return ntoh_u64(v); }

bool recvExact(socket_t s, void* buf, std::size_t n) {
    auto* p = static_cast<char*>(buf);
    std::size_t got = 0;
    while (got < n) {
        int r = ::recv(s, p + got, static_cast<int>(n - got), 0);
        if (r <= 0) return false;
        got += static_cast<std::size_t>(r);
    }
    return true;
}

bool sendAll(socket_t s, const void* buf, std::size_t n) {
    const auto* p = static_cast<const char*>(buf);
    std::size_t sent = 0;
    while (sent < n) {
        int r = ::send(s, p + sent, static_cast<int>(n - sent), 0);
        if (r <= 0) return false;
        sent += static_cast<std::size_t>(r);
    }
    return true;
}