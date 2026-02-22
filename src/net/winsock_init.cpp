#include "net/winsock_init.h"

#define WIN32_LEAN_AND_MEAN
#include <winsock2.h>
#include <ws2tcpip.h>

WinsockInit::WinsockInit() {
    WSADATA wsa{};
    int r = WSAStartup(MAKEWORD(2, 2), &wsa);
    ok_ = (r == 0);
}

WinsockInit::~WinsockInit() {
    if (ok_) WSACleanup();
}