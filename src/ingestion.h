#pragma once

#include "dsa/RingBuffer.h"
#include <thread>



struct Tick {
    uint64_t ts;        
    uint64_t price;     
    uint32_t symbol_id; 
    uint32_t size;      
};

/*
Ingestor responsible for running threads for ingestion
*/

template<typename T, size_t SIZE>
class Ingestion {

    std::array<std::thread, 1> producers {};
    std::array<std::thread, 3> consumers {};


    RingBuffer<T, SIZE> ring_buffer{};

    bool producer();
    void consumer();
    
public:

    Ingestion() {
        
        for(auto& p : producers) {
            p = std::thread(&Ingestion<T, SIZE>::producer, this);
        }

        for(auto& c : consumers) {
            c = std::thread(&Ingestion<T, SIZE>::consumer, this);
        }

    }

    void start();
};


template<typename T, size_t SIZE>
bool Ingestion<T, SIZE>::producer() {

    while(true) { 
        //listen to websocket
        //ring_buffer.add();
    }


    return true;

}

template<typename T, size_t SIZE>
void Ingestion<T, SIZE>::consumer() {

    while(true) {
        ring_buffer.read();
    }

}


template<typename T, size_t SIZE>
void Ingestion<T, SIZE>::start() {

    for(auto& p : producers)
        p.join();

    for(auto& c : consumers)
        c.join();

}
