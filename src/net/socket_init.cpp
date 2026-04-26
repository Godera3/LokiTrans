#include "net/socket_init.h"

#ifdef _WIN32
#include <winsock2.h>

SocketInit::SocketInit() {
    WSADATA wsa{};
    int r = WSAStartup(MAKEWORD(2, 2), &wsa);
    ok_ = (r == 0);
}

SocketInit::~SocketInit() {
    if (ok_) WSACleanup();
}

#else

#include <unistd.h>

SocketInit::SocketInit() : ok_(true) {}

SocketInit::~SocketInit() {}

#endif