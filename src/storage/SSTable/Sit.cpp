
#include <print>
#include <algorithm>
#include "Sit.h"
#include "../Layout/Block.h"
#include "FileMetadata.h"



void Sit::create_blocks(const std::vector<ChunkSort>& sorted_chunks) {
    
    if(sorted_chunks.empty()) return;

    storage.blocks.clear();
    storage.blocks.emplace_back();

    size_t block_idx {0}, symbol_id = sorted_chunks[0].symbol_id;

    FileMetadata* metadata = &storage.file_metadata[symbol_id];
    metadata->block_data.emplace_back();
       
    size_t global_block_idx {0};

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

                //number of blocks this symbol_id spans
                metadata = &storage.file_metadata[sorted_chunks[i].symbol_id];
                metadata->block_data.emplace_back();
                global_block_idx++;
            }


            storage.blocks.back().elements = block_idx;
            storage.blocks.emplace_back();
            
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
        
        Block& curr_block = storage.blocks.back();
        
        curr_block.ts[block_idx] = sorted_chunks[i].ts;
        curr_block.price[block_idx] = sorted_chunks[i].price;
        curr_block.vol[block_idx] = sorted_chunks[i].vol;
        block_idx++;
       
    }

    //block_idx didnt reach BLOCK_SIZE so change size manually
    if(block_idx > 0) {
        storage.blocks.back().elements = block_idx;
    }

    // print_blocks();

}

void Sit::print_blocks() {

    int count {0};
    for(const auto& block : storage.blocks) {
        std::println("Block {}\n", ++count);

        for(size_t i {0};i < block.elements;i++) {
            std::println("Timestamp {} | Price {} | Volume {}", block.ts[i], block.price[i], block.vol[i]);
        }
    }
    std::println("{}", storage.blocks.size());
}

void Sit::sort_chunks(std::vector<ChunkSort>& sorted_chunks) {

   //Compares symbol_id then timestamp then price in ascending
   std::sort(sorted_chunks.begin(), sorted_chunks.end(), [](const ChunkSort& a, const ChunkSort& b) { 
        return std::tie(a.symbol_id, a.ts, a.price) < std::tie(b.symbol_id, b.ts, b.price);
    });
   
}











