#pragma once

#include <cstddef>
#include <array>
#include <inttypes.h>

/*
data is sorted by timestamp.
data will be stored as ts[0..N] then price[0..N] then vol[0..N]
as opposed to corresponding indexes stored together

This is the columnar design choice, which will make reads super fast
*/

#define BLOCK_SIZE 1024

/*
Block size is the limit, it is not a given that the block is full
*/
struct Block {
    std::array<uint64_t, BLOCK_SIZE> ts{};
    std::array<uint64_t, BLOCK_SIZE> price{};
    std::array<uint32_t, BLOCK_SIZE> vol{};

    size_t elements {0};
};





