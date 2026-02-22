#include "crypto/sha256.h"
#include <cstring>

using namespace std;

namespace svanipp::crypto {

static inline uint32_t rotr(uint32_t x, uint32_t n) {
    return (x >> n) | (x << (32 - n));
}

static inline uint32_t ch(uint32_t x, uint32_t y, uint32_t z) {
    return (x & y) ^ (~x & z);
}

static inline uint32_t maj(uint32_t x, uint32_t y, uint32_t z) {
    return (x & y) ^ (x & z) ^ (y & z);
}

static inline uint32_t big0(uint32_t x) { return rotr(x,2) ^ rotr(x,13) ^ rotr(x,22); }
static inline uint32_t big1(uint32_t x) { return rotr(x,6) ^ rotr(x,11) ^ rotr(x,25); }
static inline uint32_t sml0(uint32_t x) { return rotr(x,7) ^ rotr(x,18) ^ (x >> 3); }
static inline uint32_t sml1(uint32_t x) { return rotr(x,17) ^ rotr(x,19) ^ (x >> 10); }

static const uint32_t K[64] = {
    0x428a2f98u,0x71374491u,0xb5c0fbcfu,0xe9b5dba5u,0x3956c25bu,0x59f111f1u,0x923f82a4u,0xab1c5ed5u,
    0xd807aa98u,0x12835b01u,0x243185beu,0x550c7dc3u,0x72be5d74u,0x80deb1feu,0x9bdc06a7u,0xc19bf174u,
    0xe49b69c1u,0xefbe4786u,0x0fc19dc6u,0x240ca1ccu,0x2de92c6fu,0x4a7484aau,0x5cb0a9dcu,0x76f988dau,
    0x983e5152u,0xa831c66du,0xb00327c8u,0xbf597fc7u,0xc6e00bf3u,0xd5a79147u,0x06ca6351u,0x14292967u,
    0x27b70a85u,0x2e1b2138u,0x4d2c6dfcu,0x53380d13u,0x650a7354u,0x766a0abbu,0x81c2c92eu,0x92722c85u,
    0xa2bfe8a1u,0xa81a664bu,0xc24b8b70u,0xc76c51a3u,0xd192e819u,0xd6990624u,0xf40e3585u,0x106aa070u,
    0x19a4c116u,0x1e376c08u,0x2748774cu,0x34b0bcb5u,0x391c0cb3u,0x4ed8aa4au,0x5b9cca4fu,0x682e6ff3u,
    0x748f82eeu,0x78a5636fu,0x84c87814u,0x8cc70208u,0x90befffau,0xa4506cebu,0xbef9a3f7u,0xc67178f2u
};

Sha256::Sha256()
: bitlen_(0), bufferLen_(0) {
    state_[0]=0x6a09e667u; state_[1]=0xbb67ae85u; state_[2]=0x3c6ef372u; state_[3]=0xa54ff53au;
    state_[4]=0x510e527fu; state_[5]=0x9b05688cu; state_[6]=0x1f83d9abu; state_[7]=0x5be0cd19u;
    memset(buffer_, 0, sizeof(buffer_));
}

void Sha256::update(const void* data, size_t len) {
    const auto* p = static_cast<const uint8_t*>(data);
    while (len > 0) {
        size_t space = 64 - bufferLen_;
        size_t take = (len < space) ? len : space;
        memcpy(buffer_ + bufferLen_, p, take);
        bufferLen_ += take;
        p += take;
        len -= take;

        if (bufferLen_ == 64) {
            transform(buffer_);
            bitlen_ += 512;
            bufferLen_ = 0;
        }
    }
}

void Sha256::final(uint8_t out32[32]) {
    // total bits includes buffered bytes
    uint64_t totalBits = bitlen_ + static_cast<uint64_t>(bufferLen_) * 8;

    // append 0x80
    buffer_[bufferLen_++] = 0x80;

    // pad with zeros until 56 bytes
    if (bufferLen_ > 56) {
        while (bufferLen_ < 64) buffer_[bufferLen_++] = 0x00;
        transform(buffer_);
        bufferLen_ = 0;
    }
    while (bufferLen_ < 56) buffer_[bufferLen_++] = 0x00;

    // append length in bits big-endian
    for (int i = 7; i >= 0; --i) {
        buffer_[bufferLen_++] = static_cast<uint8_t>((totalBits >> (i * 8)) & 0xFF);
    }
    transform(buffer_);

    // output big-endian
    for (int i = 0; i < 8; ++i) {
        out32[i*4 + 0] = static_cast<uint8_t>((state_[i] >> 24) & 0xFF);
        out32[i*4 + 1] = static_cast<uint8_t>((state_[i] >> 16) & 0xFF);
        out32[i*4 + 2] = static_cast<uint8_t>((state_[i] >> 8) & 0xFF);
        out32[i*4 + 3] = static_cast<uint8_t>((state_[i]) & 0xFF);
    }
}

void Sha256::transform(const uint8_t block[64]) {
    uint32_t w[64];

    for (int i = 0; i < 16; ++i) {
        w[i] = (static_cast<uint32_t>(block[i*4 + 0]) << 24) |
               (static_cast<uint32_t>(block[i*4 + 1]) << 16) |
               (static_cast<uint32_t>(block[i*4 + 2]) << 8)  |
               (static_cast<uint32_t>(block[i*4 + 3]));
    }
    for (int i = 16; i < 64; ++i) {
        w[i] = sml1(w[i-2]) + w[i-7] + sml0(w[i-15]) + w[i-16];
    }

    uint32_t a=state_[0], b=state_[1], c=state_[2], d=state_[3];
    uint32_t e=state_[4], f=state_[5], g=state_[6], h=state_[7];

    for (int i = 0; i < 64; ++i) {
        uint32_t t1 = h + big1(e) + ch(e,f,g) + K[i] + w[i];
        uint32_t t2 = big0(a) + maj(a,b,c);
        h = g;
        g = f;
        f = e;
        e = d + t1;
        d = c;
        c = b;
        b = a;
        a = t1 + t2;
    }

    state_[0] += a; state_[1] += b; state_[2] += c; state_[3] += d;
    state_[4] += e; state_[5] += f; state_[6] += g; state_[7] += h;
}

} // namespace svanipp::crypto