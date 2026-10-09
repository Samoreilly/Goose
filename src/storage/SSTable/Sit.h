#pragma once

#include <print>
#include <inttypes.h>

#include "../../dsa/MemTable.h"

/*
Columnar
Sorted Integer Table as opposed to sorted integer table
Keys will be the id that is converted from the ticker at ingestion stage
*/


//Templated size to take in variable "used" in chunks

struct SSTable {
    std::vector<int64_t> ts = {0};
    std::vector<uint64_t> price = {0};
    std::vector<uint32_t> vol = {0};
};

class Sit {

    SSTable table{};
    //moves sorted chunk data to columnar arrays
    void transfer_to_ss(const std::vector<ChunkSort>& m);

public:

    
    //must provide exact types
    explicit Sit(const std::vector<ChunkSort>&& sorted_chunks, MemTable& m) {
        m.sort();

        table.ts.resize(sorted_chunks.size(), 0);
        table.price.resize(sorted_chunks.size(), 0);
        table.vol.resize(sorted_chunks.size(), 0);

        transfer_to_ss(sorted_chunks);

        std::println("Transferred to SSTable");
        
    }
    
};
