#pragma once
 
#include <print>
#include <inttypes.h>
#include <vector>
#include <shared_mutex>

#include "../Layout/Storage.h"
#include "../../dsa/MemTable.h"
#include "../Layout/Block.h"
#include "../Compression/Compress.h"
#include "SSTable.h"

/*
Columnar
Sorted Integer Table as opposed to sorted integer table
Keys will be the id that is converted from the ticker at ingestion stage
*/

class Sit {

    Sit() = delete;

    void print_blocks(SSTable& storage);
    
    //calculates how much blocks are needed - used for blocks.reserve()
    inline uint32_t calc_reserve_for_blocks(const std::vector<SortMetadata>& counts) {
        size_t blocks {0};
        for (const auto count : counts) {
            if (count.data == 0) continue;
            blocks += (count.data + BLOCK_SIZE - 1) / BLOCK_SIZE;
        }
        return blocks;
    }

public:

    //moves sorted chunk data to columnar arrays (storage.mu held by build())
    static void create_blocks(std::unique_ptr<SSTable>& sstable, const std::vector<ChunkSort>& m);

    Sit(Storage& storage, std::vector<ChunkSort>&& sorted_chunks, std::vector<SortMetadata>&& counts, uint32_t n) {
        
        if(sorted_chunks.empty()) return;

        //sort outside the lock so queries dont wait
        sort_chunks(sorted_chunks, counts);

        //block readers until SSTable has been moved to storage.sstables
        std::unique_lock<std::shared_mutex> mu(storage.mu);
        auto* sstable = build_sstable(storage, sorted_chunks, counts, n);
        
        //compression stage
        compression::compress(storage, sstable);
    }

    //builds and adds SSTable to global SSTable vector and return reference
    SSTable* build_sstable(
        Storage& storage, std::vector<ChunkSort>& sorted_chunks,
        const std::vector<SortMetadata>& counts, uint32_t n
    );

    void sort_chunks(std::vector<ChunkSort>& sorted_chunks, const std::vector<SortMetadata>& counts);

    //symbol + timestamp lookup. locks storage.mu, skips on symbol scope
    //then block scope min/max, returns matching rows in ts order.
    static std::vector<ChunkSort> lookup(Storage& storage, uint32_t symbol_id, uint64_t t_start, uint64_t t_end);
    
};
