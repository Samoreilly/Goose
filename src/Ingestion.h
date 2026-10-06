#pragma once

#include <cstring>
#include <optional>
#include <thread>
#include <assert.h>

#include "dsa/MemTable.h"
#include "wal/Wal.h"
#include "dsa/RingBuffer.h"
#include "helpers/Convert.h"

/*
Ingestor responsible for orchestration && ingestion threads
*/

template<size_t SIZE>
class Ingestion {

    Wal& wal;
    MemTable& mem_table;
    std::thread producers;
    std::thread consumers;

    RingBuffer<Data, SIZE> ring_buffer{};
    void producer();
    void consumer();
    

    uint64_t assign_tickr(std::string_view ticker);


public:

    Ingestion(Wal& w, MemTable& mem) : wal(w), mem_table(mem) {}

    void start();
};


template<size_t SIZE>
void Ingestion<SIZE>::producer() {

    for(int i {0};i < 20;i++) {
        //will change to websocket connection
        Data data (i, i + i, i + 8, "AAPL"); 
        
        size_t spins {0};
        while(!ring_buffer.add(std::move(data))) {
            if(spins++ < ALLOWED_SPINS) {
                _mm_pause();
            }else {
                std::this_thread::yield();
                spins = 0;
            }
        }
    }

}

template<size_t SIZE>
void Ingestion<SIZE>::consumer() {

    for(int i {0};i < 20;i++) {
        Data&& data = ring_buffer.read();

        //it will return inserted == false if already key exists
        //This only does 1 lookup, last implementation did 3
        auto [it, inserted] = ticker_to_id.try_emplace(data.symbol, ticker_count);
        if (inserted) {
            id_to_ticker.insert({ticker_count, data.symbol});
            ticker_count++;
        }

        uint32_t symbol_id = it->second;

        Tick tick{data.ts, data.price, data.vol, symbol_id};
        //append to WAL first for durability incase crash happens
        wal.append(tick);
        mem_table.append(tick);
    }
}


template<size_t SIZE>
void Ingestion<SIZE>::start() {
    producers = std::thread(&Ingestion::producer, this);
    consumers = std::thread(&Ingestion::consumer, this);
    producers.join();
    consumers.join();
}

//Moves the ticker into an packed 8 byte integer
template<size_t SIZE>
uint64_t Ingestion<SIZE>::assign_tickr(std::string_view ticker) {

    //checks if string will fit into our 8 byte integer
    assert(ticker.size() <= 8 && "Ticker exceeded 8 byte size");

    uint64_t res {0};
    std::memcpy(&res, ticker.data(), 8);
    
    return res;
}



