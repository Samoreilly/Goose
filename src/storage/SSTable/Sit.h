#pragma once
 
#include <print>
#include <inttypes.h>

#include "../../dsa/MemTable.h"
#include "../Layout/Block.h"
#include "../Layout/Storage.h"
/*
Columnar
Sorted Integer Table as opposed to sorted integer table
Keys will be the id that is converted from the ticker at ingestion stage
*/

class Sit {
    
    Storage storage;
    std::vector<Block> blocks;
    uint32_t n_symbols;

    //moves sorted chunk data to columnar arrays
    void create_blocks(const std::vector<ChunkSort>& m);
    void print_blocks();
    
    //calculates how much blocks are needed - used for blocks.reserve()
    uint32_t calc_reserve_for_blocks(std::vector<uint32_t>& counts) {
        size_t blocks {0};
        for (const auto count : counts) {
            if (count == 0) continue;
            blocks += (count + BLOCK_SIZE - 1) / BLOCK_SIZE;
        }
        return blocks;

    }

public:

    //Only pass rvalues                 number of unique symbols in this batch
    Sit(std::vector<ChunkSort>&& sorted_chunks, std::vector<uint32_t>&& counts, uint32_t n) : n_symbols(n) {
        
        n_symbols = 1;//NOTE:REMOVE, used to compiler w no errors
        blocks.reserve(calc_reserve_for_blocks(counts));

        sort_chunks(sorted_chunks);
        create_blocks(sorted_chunks);

        std::println("Transferred to SSTable");
    }

    Sit(std::vector<ChunkSort>& m) = delete;
    Sit(const std::vector<ChunkSort>& m) = delete;
    
    void sort_chunks(std::vector<ChunkSort>& sorted_chunks);

};
