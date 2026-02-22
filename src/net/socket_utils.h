#pragma once
#include <cstddef>
#include <cstdint>

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <winsock2.h>
using socket_t = SOCKET;
static constexpr socket_t INVALID_SOCKET_T = INVALID_SOCKET;
#else
using socket_t = int;
static constexpr socket_t INVALID_SOCKET_T = -1;
#endif

bool recvExact(socket_t s, void* buf, std::size_t n);
bool sendAll(socket_t s, const void* buf, std::size_t n);

uint16_t ntoh_u16(uint16_t v);
uint16_t hton_u16(uint16_t v);
uint64_t ntoh_u64(uint64_t v);
uint64_t hton_u64(uint64_t v);