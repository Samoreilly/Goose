
#include <algorithm>

#include "Sit.h"


void Sit::transfer_to_ss(const std::vector<ChunkSort>& sorted_chunks) {

    for(size_t i {0};i < sorted_chunks.size();i++) {
        table.ts[i] = sorted_chunks[i].ts;
        table.price[i] = sorted_chunks[i].price;
        table.vol[i] = sorted_chunks[i].vol;
    }
 
}

//This should be really fast because timestamps are mostly in order
//This is only incase the websocket received them out of order
//Hard to know if this is actually needed until proper end to end testing
void Sit::sort_chunks(std::vector<ChunkSort>& sorted_chunks) {

   //Oldest first - ascending
   std::sort(sorted_chunks.begin(), sorted_chunks.end(), [](ChunkSort& a, ChunkSort& b) { return a.ts < b.ts; });
   

}
