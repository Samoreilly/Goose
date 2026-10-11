#pragma once
 
#include <print>
#include <inttypes.h>
#include <vector>
#include <shared_mutex>

#include "../Layout/Storage.h"
#include "../../dsa/MemTable.h"
#include "../Layout/Block.h"
#include "SSTable.h"

/*
Columnar
Sorted Integer Table as opposed to sorted integer table
Keys will be the id that is converted from the ticker at ingestion stage
*/

class Sit {

    Sit() = delete;

    //moves sorted chunk data to columnar arrays (storage.mu held by build())
    void create_blocks(std::unique_ptr<SSTable>& sstable, const std::vector<ChunkSort>& m);
    void print_blocks(SSTable& storage);
    
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
        
        if(sorted_chunks.empty()) return;

        auto sstable = std::make_unique<SSTable>();
        
        //sort outside the lock so queries dont wait
        sort_chunks(sorted_chunks);

        std::unique_lock<std::shared_mutex> mu(storage.mu);

        //keep original size and calculate new additional size
        sstable->blocks.reserve(sstable->blocks.size() + calc_reserve_for_blocks(counts));
        
        sstable->file_metadata.reserve(sstable->file_metadata.size() + n);

        create_blocks(sstable, sorted_chunks);

        storage.sstables.emplace_back(std::move(sstable));

        std::println("Transferred to SSTable");
    }

    //symbol + timestamp lookup. locks storage.mu, skips on symbol scope
    //then block scope min/max, returns matching rows in ts order.
    static std::vector<ChunkSort> lookup(Storage& storage, uint32_t symbol_id, uint64_t t_start, uint64_t t_end);
    
    void sort_chunks(std::vector<ChunkSort>& sorted_chunks);

};
