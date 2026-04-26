#pragma once

#include "socket_utils.h"

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <winsock2.h>
#include <ws2tcpip.h>
#else
#include <unistd.h>
#endif

class SocketInit {
public:
    SocketInit();
    ~SocketInit();

    SocketInit(const SocketInit&) = delete;
    SocketInit& operator=(const SocketInit&) = delete;

    bool ok() const { return ok_; }

private:
    bool ok_{false};
};

inline void close_socket(socket_t s) {
#ifdef _WIN32
    ::closesocket(s);
#else
    ::close(s);
#endif
}