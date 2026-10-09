#pragma once

#include <chrono>
#include <vector>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <inttypes.h>
#include <functional>

#include "../storage/BackgroundWork.h"
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
class MemTable;
void cleanup_memtable(MemTable& table);
 

class MemTable {
  

   //buf -> sorted on flush, then both cleared
    void print_chunksort(const std::vector<ChunkSort>& chunk_sort);
    std::thread cleanup_thread;

public:

    MemTable() {
        cleanup_thread = std::thread(cleanup_memtable, std::ref(*this));
    }

    ~MemTable() {
       { 
            std::lock_guard<std::mutex> lk(mu); 
            //signals cleanup thread to stop
            stop = true;
        }
        
        cond_var.notify_all();
        
        if(cleanup_thread.joinable()) {
            cleanup_thread.join();
        }
    }

    std::vector<TickerBuffer> buf {15000};
    std::vector<ChunkSort> sorted;
    
    std::condition_variable cond_var;
    
    std::mutex mu;
    static constexpr int MAX_TICKS {10000};//NOTE: decide on a optimal value
    //Heuristics to know when to flush
    size_t total_ticks {0};
    std::chrono::time_point<std::chrono::steady_clock> last_append;
    bool stop {false};

    void reset_memtable() {
        buf.assign(15000, TickerBuffer{});
        
        total_ticks = 0;
        total_size = 0;
        last_append = std::chrono::steady_clock::now();
    }


    void append(Tick& t);
    inline TickerBuffer& get(uint32_t symbol_id);
     
    //calculated during sort()
    size_t total_size {0};

    //occurs when flush is triggered
    void print();
};



