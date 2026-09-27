#pragma once

#include <atomic>
#include <thread>

/*
Will be used in INGESTION & possibly used in calculating moving averages
*/

template<typename T, size_t SIZE>
class RingBuffer {

    T buffer[SIZE];
    alignas(8) std::atomic<size_t> /* write */ head {0}, /* read */ tail {0};

public:

    bool add(const T& data);
    T read();
};

/*
This is a SPMC, the add function is more or less non atomic as its only single producer
The read function relies on CAS to handle multiple threads changing values
*/

template<typename T, size_t SIZE>
bool RingBuffer<T, SIZE>::add(const T& data) {
    
    size_t curr_head = head.load(std::memory_order_relaxed);
    size_t curr_tail = tail.load(std::memory_order_acquire);

    //check if full. This is unlikely as there is a high consumer to producer ratio
    if(curr_head - curr_tail >= SIZE) [[unlikely]] return false;

    buffer[curr_head & (SIZE - 1)] = data;

    //essentially publishes all writes including previous writes like for e.g. line 36
    head.store(curr_head + 1, std::memory_order_release);

    return true;
}

template<typename T, size_t SIZE>
T RingBuffer<T, SIZE>::read(){
    
    while(true) {
        
        size_t curr_tail = tail.load(std::memory_order_relaxed);
        //acquire so it reads fresh value
        size_t curr_head = head.load(std::memory_order_acquire);
        
        if(curr_tail >= curr_head) { std::this_thread::yield(); continue; }

        //checks if tail is equal to curr_tail
        //has if it has not been changed from intialisation until now
        if(tail.compare_exchange_weak(curr_tail, curr_tail + 1, std::memory_order::release, std::memory_order_relaxed)) {
            return buffer[curr_tail & (SIZE - 1)]; 
        }


    }

}


