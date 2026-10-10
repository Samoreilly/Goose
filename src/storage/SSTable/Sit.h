#pragma once

#include <print>
#include <inttypes.h>

#include "../../dsa/MemTable.h"
#include "../Layout/Block.h"

/*
Columnar
Sorted Integer Table as opposed to sorted integer table
Keys will be the id that is converted from the ticker at ingestion stage
*/

class Sit {

    std::vector<Block> blocks;

    //moves sorted chunk data to columnar arrays
    void create_blocks(const std::vector<ChunkSort>& m);
    void print_blocks();

public:

    
    //must provide exact types
    explicit Sit(std::vector<ChunkSort>&& sorted_chunks){
        std::println("SSTable flush{}", sorted_chunks.size());
        
        sort_chunks(sorted_chunks);
        create_blocks(sorted_chunks);

        std::println("Transferred to SSTable");
    }
    
    void sort_chunks(std::vector<ChunkSort>& sorted_chunks);

};
