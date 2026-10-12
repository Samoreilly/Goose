
#include <print>
#include <algorithm>
#include <mutex>
#include <tuple>
#include <ranges>
#include <mutex>
#include <shared_mutex>

#include "Sit.h"
#include "../Layout/Block.h"
#include "FileMetadata.h"
#include "../Layout/Storage.h"

void Sit::create_blocks(std::unique_ptr<SSTable>& sstable, const std::vector<ChunkSort>& sorted_chunks) {
    //mutex held by build() storage.mu

    sstable->blocks.emplace_back();
    size_t global_block_idx = sstable->blocks.size() - 1;

    size_t block_idx {0};
    uint32_t symbol_id = sorted_chunks[0].symbol_id;

    FileMetadata* metadata = &sstable->file_metadata[symbol_id];
    metadata->block_data.emplace_back();

    for(size_t i {0};i < sorted_chunks.size();i++) {

        const bool diff_symbol = symbol_id != sorted_chunks[i].symbol_id;
        //create new block and if id is not same as prev, create new block
        if(block_idx == BLOCK_SIZE || diff_symbol) [[unlikely]] {
           
            
            //block is full and same symbol as last
            if(block_idx == BLOCK_SIZE && !diff_symbol) {
                metadata->block_data.emplace_back();
                global_block_idx++;
            }

            //different symbol so reset everything
            if(diff_symbol) {
                metadata = &sstable->file_metadata[sorted_chunks[i].symbol_id];
                metadata->block_data.emplace_back();
                global_block_idx++;
            }


            sstable->blocks.back().elements = block_idx;
            sstable->blocks.emplace_back();
            
            block_idx = 0;
            symbol_id = sorted_chunks[i].symbol_id;
        }
        
        //globals
        metadata->max_ts = std::max(metadata->max_ts, sorted_chunks[i].ts);
        metadata->min_ts = std::min(metadata->min_ts, sorted_chunks[i].ts);        
        metadata->max_price = std::max(metadata->max_price, sorted_chunks[i].price);
        metadata->min_price = std::min(metadata->min_price, sorted_chunks[i].price);

        BlockMeta* bmeta = &metadata->block_data.back();

        bmeta->block_id = global_block_idx;

        //update locals
        bmeta->bmax_ts = std::max(bmeta->bmax_ts, sorted_chunks[i].ts);
        bmeta->bmin_ts = std::min(bmeta->bmin_ts, sorted_chunks[i].ts);
        bmeta->bmax_price = std::max(bmeta->bmax_price, sorted_chunks[i].price);
        bmeta->bmin_price = std::min(bmeta->bmin_price, sorted_chunks[i].price);
        
        Block& curr_block = sstable->blocks.back();
        
        curr_block.ts[block_idx] = sorted_chunks[i].ts;
        curr_block.price[block_idx] = sorted_chunks[i].price;
        curr_block.vol[block_idx] = sorted_chunks[i].vol;
        block_idx++;
       
    }

    //block_idx didnt reach BLOCK_SIZE so change size manually
    if(block_idx > 0) {
        sstable->blocks.back().elements = block_idx;
    }
}


std::vector<ChunkSort> Sit::lookup(Storage& storage, uint32_t symbol_id, uint64_t t_start, uint64_t t_end) {
    std::vector<ChunkSort> out;

    //read only
    std::shared_lock<std::shared_mutex> lk(storage.mu);

    for(const auto& index : std::views::reverse(storage.sstables)) {

        auto it = index->file_metadata.find(symbol_id);
        
        //symbol not in sstable
        if(it == index->file_metadata.end()) {
            continue;
        }

        const FileMetadata& meta = it->second;

        //symbol scope skip
        if(t_end < meta.min_ts || t_start > meta.max_ts) {
            continue;
        }

        for(const BlockMeta& bmeta : meta.block_data) {
            //block scope skip
            if(t_end < bmeta.bmin_ts || t_start > bmeta.bmax_ts) continue;

            const Block& block = index->blocks[bmeta.block_id];
            for(size_t i {0};i < block.elements;i++) {
                const uint64_t ts = block.ts[i];
                //is ts is within range
                if(ts >= t_start && ts <= t_end) {
                    out.push_back({ts, block.price[i], block.vol[i], symbol_id});
                }
            }
        }

    }

    return out;
}

void Sit::print_blocks(SSTable& storage) {

    std::lock_guard<std::mutex> lk(storage.mu);

    int count {0};
    for(const auto& block : storage.blocks) {
        std::println("Block {}\n", ++count);

        for(size_t i {0};i < block.elements;i++) {
            std::println("Timestamp {} | Price {} | Volume {}", block.ts[i], block.price[i], block.vol[i]);
        }
    }
    std::println("{}", storage.blocks.size());
}

//This should be really fast because timestamps are mostly in order
//This is only incase the websocket received them out of order
//Hard to know if this is actually needed until proper end to end testing
void Sit::sort_chunks(std::vector<ChunkSort>& sorted_chunks) {

   //Compares symbol_id then timestamp then price in ascending
   std::sort(sorted_chunks.begin(), sorted_chunks.end(), [](const ChunkSort& a, const ChunkSort& b) { 
        return std::tie(a.symbol_id, a.ts, a.price) < std::tie(b.symbol_id, b.ts, b.price);
   });
   
}
