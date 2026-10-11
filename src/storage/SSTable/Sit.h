#pragma once
 
#include <print>
#include <inttypes.h>
#include <vector>

#include "../../dsa/MemTable.h"
#include "../Layout/Block.h"
#include "../Layout/Storage.h"

/*
Columnar
Sorted Integer Table as opposed to sorted integer table
Keys will be the id that is converted from the ticker at ingestion stage
*/

class Sit {

    Sit() = delete;

    //moves sorted chunk data to columnar arrays (storage.mu held by build())
    void create_blocks(Storage& storage, const std::vector<ChunkSort>& m);
    void print_blocks(Storage& storage);
    
    //calculates how much blocks are needed - used for blocks.reserve()
    uint32_t calc_reserve_for_blocks(const std::vector<uint32_t>& counts) {
        size_t blocks {0};
        for (const auto count : counts) {
            if (count == 0) continue;
            blocks += (count + BLOCK_SIZE - 1) / BLOCK_SIZE;
        }
        return blocks;

    }

public:

    Sit(Storage& storage, std::vector<ChunkSort>&& sorted_chunks, std::vector<uint32_t>&& counts, uint32_t n) {
        //sort outside the lock so queries dont wait
        sort_chunks(sorted_chunks);

        if(sorted_chunks.empty()) return;

        std::lock_guard<std::mutex> lk(storage.mu);

        //keep original size and calculate new additional size
        storage.blocks.reserve(storage.blocks.size() + calc_reserve_for_blocks(counts));
        
        storage.file_metadata.reserve(storage.file_metadata.size() + n);

        create_blocks(storage, sorted_chunks);

        std::println("Transferred to SSTable");
    }

    //symbol + timestamp lookup. locks storage.mu, skips on symbol scope
    //then block scope min/max, returns matching rows in ts order.
    static std::vector<ChunkSort> lookup(Storage& storage, uint32_t symbol_id, uint64_t t_start, uint64_t t_end);
    
    void sort_chunks(std::vector<ChunkSort>& sorted_chunks);

};
