#pragma once

#include "wal/wal.h"
#include "dsa/RingBuffer.h"
#include <cstring>
#include <thread>
#include <assert.h>


/*
Ingestor responsible for running threads for ingestion
*/

template<size_t SIZE>
class Ingestion {

    Wal wal;
    std::thread producers;
    std::thread consumers;

    RingBuffer<Data, SIZE> ring_buffer{};
    void producer();
    void consumer();
    

    uint64_t assign_tickr(std::string_view ticker);


public:

    Ingestion(Wal& w) : wal(w) {
 

    }

    void start();
};


template<size_t SIZE>
void Ingestion<SIZE>::producer() {

    while(true) {
        //will change to websocket connection
        Data data = {}; 

        while(!ring_buffer.add(std::move(data))) {
            std::this_thread::yield();
        }

    }
}

template<size_t SIZE>
void Ingestion<SIZE>::consumer() {

    while(true) {
        Data&& data = ring_buffer.read();
        const Tick tick{data.ts, data.price, assign_tickr(data.symbol), data.size};
        wal.append(tick);  
    }

}


template<size_t SIZE>
void Ingestion<SIZE>::start() {
    producers.join();
    consumers.join();

}

//Moves the ticker into an 8 byte integer
template<size_t SIZE>
uint64_t Ingestion<SIZE>::assign_tickr(std::string_view ticker) {

    //checks if string will fit into our 8 byte integer
    assert(ticker.size() <= 8 && "Ticker exceeded 8 byte size");

    uint64_t res {0};
    std::memcpy(&res, ticker.data(), 8);
    
    return res;
}



