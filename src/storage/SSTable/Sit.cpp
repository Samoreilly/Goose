
#include "Sit.h"

void Sit::transfer_to_ss(const std::vector<ChunkSort>& sorted_chunks) {

    for(size_t i {0};i < sorted_chunks.size();i++) {
        table.ts[i] = sorted_chunks[i].ts;
        table.price[i] = sorted_chunks[i].price;
        table.vol[i] = sorted_chunks[i].vol;
    }

   
    
}


