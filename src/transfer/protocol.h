#pragma once
#include <cstdint>

namespace svanipp::proto {
    static constexpr char MAGIC[8] = { 'S','V','A','N','I','P','P','1' };
    static constexpr uint16_t VERSION = 1;

#pragma pack(push, 1)
    struct HeaderFixed {
        char magic[8];
        uint16_t version;      // network order
        uint16_t filename_len; // network order
        uint64_t file_size;    // network order
    };
#pragma pack(pop)
}