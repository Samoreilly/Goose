#pragma once

#include <vector>
#include <inttypes.h>

#include "../DataTypes.h"

struct Chunk {
    static constexpr int N = 4096;
    uint64_t ts[N] = {0};        
    uint64_t price[N] = {0};     
    uint32_t vol[N] = {0};
    size_t idx {0};//index of free slot
};
struct TickerBuffer {
    std::vector<Chunk> chunks;
    uint64_t last_ts;
    uint64_t last_price;
    uint32_t last_vol;
    uint64_t count {0}; //how much data was sent to this ticker
};

class MemTable {

    std::vector<TickerBuffer> buf;

public:

    MemTable() { }

    void append(Tick& t);
    TickerBuffer& get(uint32_t symbol_id);
};
