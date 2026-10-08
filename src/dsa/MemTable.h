#pragma once

#include <vector>
#include <inttypes.h>

#include "../DataTypes.h"
#include "MovingAverage.h"

struct ChunkSort {
    uint64_t ts;
    uint64_t price;
    uint64_t vol;
};

struct Chunk {
    static constexpr int N = 4096;
    uint64_t ts[N] = {0};        
    uint64_t price[N] = {0};     
    uint32_t vol[N] = {0};
    size_t idx {0};//index of free slot
};

struct TickerBuffer {
    std::vector<Chunk> chunks;
    
    MovingAverage<30> monthly;
    MovingAverage<365> yearly;

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

    //A heuristic to know when to flush
    size_t total_ticks {0};
    std::vector<TickerBuffer> buf {15000};
    //buf -> sorted on flush, then both cleared
    std::vector<ChunkSort> sorted {15000};
    void print_chunksort(const std::vector<ChunkSort>& chunk_sort);

public:

    MemTable() {}

    void append(Tick& t);
    inline TickerBuffer& get(uint32_t symbol_id);
     
    void sort();
    //calculated during sort()
    size_t total_size {0};

    //occurs when flush is triggered
    void print();
};
