#pragma once

#include <cstddef>
#include <print>
#include <inttypes.h>

/*
Calculates moving average using a ring buffer, it eventually wraps around itself, removing the last value(1st)
and replacing with current(30th)
*/

template<size_t SIZE>
class MovingAverage {

    uint64_t moving_average[SIZE] = {0};
    uint64_t running_sum {0};
    size_t head {0};

public:

    MovingAverage() {}

    void add(uint64_t price) {

        const size_t i = head % SIZE;

        running_sum -= moving_average[i];
        moving_average[i] = price;
        running_sum += price;
    
        std::println("{}", running_sum);
        head++;
    }

    uint64_t get() const {
        return running_sum / SIZE;
    }

   
};


