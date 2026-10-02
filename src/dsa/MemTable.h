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
    uint64_t last_ts {0};
    uint64_t last_price {0};
    uint32_t last_vol {0};
    uint64_t count {0}; //how much data was sent to this ticker
};

/*
This Memtable contains TickerBuffer where a slot contains Chunk
Chunk holds recent stock market data, that can be queried fast
Data is stored columnarly for locality, which should make specific queries a bit faster
*/
class MemTable {

    std::vector<TickerBuffer> buf {15000};

public:

    MemTable() {}

    void append(Tick& t);
    TickerBuffer& get(uint32_t symbol_id);
    
    void print();
};
