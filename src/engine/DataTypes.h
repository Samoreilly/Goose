#pragma once

#include <inttypes.h>
#include <string>

struct Data {
    uint64_t ts;
    uint64_t price;
    uint32_t vol;
    std::string symbol;
};

struct Tick {
    uint64_t ts;
    uint64_t price;
    uint32_t vol;
    uint32_t symbol_id;
};
