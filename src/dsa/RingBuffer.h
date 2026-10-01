#pragma once

#include <atomic>
#include <thread>
#include <xmmintrin.h>
#define ALLOWED_SPINS 60

/*
Will be used in INGESTION & possibly used in calculating moving averages
*/
template<typename T, size_t SIZE>
class RingBuffer {

    T buffer[SIZE];
    alignas(8) std::atomic<size_t> /* write */ head {0}, /* read */ tail {0};
    
    uint64_t spins {0};

public:

    bool add(T&& data);
    T read();
};

/*
This is a SPMC, the add function is more or less non atomic as its only single producer
The read function relies on CAS to handle multiple threads changing values
*/

template<typename T, size_t SIZE>
bool RingBuffer<T, SIZE>::add(T&& data) {
    
    size_t curr_head = head.load(std::memory_order_relaxed);
    size_t curr_tail = tail.load(std::memory_order_acquire);

    //check if full. This is unlikely as there is a high consumer to producer ratio
    if(curr_head - curr_tail >= SIZE) [[unlikely]] return false;

    buffer[curr_head & (SIZE - 1)] = std::move(data);
    
    //essentially publishes all writes including previous writes like for e.g. line 36
    head.store(curr_head + 1, std::memory_order_release);
    head.notify_one();

    return true;
}

template<typename T, size_t SIZE>
T RingBuffer<T, SIZE>::read(){
     
    size_t curr_tail = tail.load(std::memory_order_relaxed);
    
    //this loops blocks thread if curr == head, aka no data being produced 
    //acquire so it reads fresh value 
    while(curr_tail == head.load(std::memory_order_acquire)) {
        
        if(spins++ < ALLOWED_SPINS) {
            //slows down instructions, stops the while loop from overloading the cpu
            _mm_pause();
        }else {
            head.wait(curr_tail, std::memory_order_acquire);
            spins = 0;
        }
    }

    T out = std::move(buffer[curr_tail & (SIZE - 1)]);
    tail.store(curr_tail + 1, std::memory_order_release);

    return out;
}


