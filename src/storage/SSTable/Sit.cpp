
#include <print>
#include <algorithm>

#include "Sit.h"
#include "../Layout/Block.h"


void Sit::create_blocks(const std::vector<ChunkSort>& sorted_chunks) {
   
    if(sorted_chunks.empty()) return;

    blocks.clear();
    //calculates number of blocks needed for sorted_chunks
    blocks.reserve((sorted_chunks.size() + BLOCK_SIZE - 1) / BLOCK_SIZE);

    blocks.emplace_back();

    size_t block_idx {0};
    for(size_t i {0};i < sorted_chunks.size();i++) {

        if(block_idx == BLOCK_SIZE) [[unlikely]] {
            blocks.back().elements = BLOCK_SIZE;
            
            blocks.emplace_back();
            block_idx = 0;
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

//This should be really fast because timestamps are mostly in order
//This is only incase the websocket received them out of order
//Hard to know if this is actually needed until proper end to end testing
void Sit::sort_chunks(std::vector<ChunkSort>& sorted_chunks) {

   //Oldest first - ascending
   std::sort(sorted_chunks.begin(), sorted_chunks.end(), [](ChunkSort& a, ChunkSort& b) { return a.ts < b.ts; });
   
}
