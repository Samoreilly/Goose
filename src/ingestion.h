#pragma once

#include <cstring>
#include <optional>
#include <thread>
#include <assert.h>

#include "dsa/MemTable.h"
#include "wal/wal.h"
#include "dsa/RingBuffer.h"
#include "helpers/Convert.h"

/*
Ingestor responsible for running threads for ingestion
*/

template<size_t SIZE>
class Ingestion {

    Wal wal;
    MemTable mem_table;
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

    while(true) {

        //will change to websocket connection
        Data data {}; 

        while(!ring_buffer.add(std::move(data))) {
            std::this_thread::yield();
        }

    }
}

template<size_t SIZE>
void Ingestion<SIZE>::consumer() {

    while(true) {

        Data&& data = ring_buffer.read();
        uint32_t symbol_id {0};

        //gets the corresponding unique id for the ticker
        if(convert.find(data.symbol) != convert.end()) {
            symbol_id = convert[data.symbol];
        }else {
            convert[data.symbol] = ticker_count++; 
        }

        Tick tick{data.ts, data.price, data.vol, symbol_id};
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



