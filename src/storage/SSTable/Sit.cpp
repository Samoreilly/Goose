
#include <print>
#include <algorithm>
#include <unordered_set>

#include "Sit.h"
#include "../Layout/Block.h"


void Sit::create_blocks(const std::vector<ChunkSort>& sorted_chunks) {
    
    if(sorted_chunks.empty()) return;

    blocks.clear();

    blocks.emplace_back();

    size_t block_idx {0}, symbol_id = sorted_chunks[0].symbol_id;
    
    for(size_t i {0};i < sorted_chunks.size();i++) {

        //create new block and if id is not same as prev, create new block
        if(block_idx == BLOCK_SIZE || symbol_id != sorted_chunks[i].symbol_id) [[unlikely]] {
            blocks.back().elements = block_idx;
            
            blocks.emplace_back();
            block_idx = 0;
            symbol_id = sorted_chunks[i].symbol_id;
        }

        Block& curr_block = blocks.back();
        
        curr_block.ts[block_idx] = sorted_chunks[i].ts;
        curr_block.price[block_idx] = sorted_chunks[i].price;
        curr_block.vol[block_idx] = sorted_chunks[i].vol;
        block_idx++;
        
       
    }

    //block_idx didnt reach BLOCK_SIZE so change size manually
    if(block_idx > 0) {
        blocks.back().elements = block_idx;
    }

    // print_blocks();

}

void Sit::print_blocks() {

    int count {0};
    for(const auto& block : blocks) {
        std::println("Block {}\n", ++count);

        for(size_t i {0};i < block.elements;i++) {
            std::println("Timestamp {} | Price {} | Volume {}", block.ts[i], block.price[i], block.vol[i]);
        }
    }
    std::println("{}", blocks.size());
}

void Sit::sort_chunks(std::vector<ChunkSort>& sorted_chunks) {

   //Compares symbol_id then timestamp then price in ascending
   std::sort(sorted_chunks.begin(), sorted_chunks.end(), [](const ChunkSort& a, const ChunkSort& b) { 
        return std::tie(a.symbol_id, a.ts, a.price) < std::tie(b.symbol_id, b.ts, b.price);
    });
   
}











