#pragma once
#include <cstdint>
#include <cstddef>

using namespace std;

namespace svanipp::crypto {

class Sha256 {
public:
    Sha256();

    void update(const void* data, size_t len);
    void final(uint8_t out32[32]); // writes 32 bytes

private:
    void transform(const uint8_t block[64]);

    uint32_t state_[8];
    uint64_t bitlen_;
    uint8_t buffer_[64];
    size_t bufferLen_;
};

} // namespace svanipp::crypto