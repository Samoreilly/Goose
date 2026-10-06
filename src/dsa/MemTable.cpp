
#include <cstdint>
#include <string.h>
#include <stddef.h>
#include <ranges>
#include <print>

#include "MemTable.h"



void MemTable::append(Tick& t) {

   TickerBuffer& slot = get(t.symbol_id);
   
   if(slot.chunks.empty() || slot.chunks.back().idx == (Chunk::N)) [[unlikely]] {
      slot.chunks.emplace_back();
   }
   
   Chunk* chunk = &slot.chunks.back();

   //adds values to chunks
   chunk->ts[chunk->idx] = t.ts;
   chunk->price[chunk->idx] = t.price;
   chunk->vol[chunk->idx] = t.vol;
   chunk->idx++;

   //updates most recent values
                                                   //pretty cool method lol
   uint8_t* dest = reinterpret_cast<uint8_t*>(&slot) + offsetof(TickerBuffer, last_ts);
   uint8_t* src = reinterpret_cast<uint8_t*>(&t);

   memcpy(dest, src, sizeof(slot.last_ts) + sizeof(slot.last_price) + sizeof(slot.last_vol)); 
   
   std::println("End of append memtable");
}

TickerBuffer& MemTable::get(uint32_t symbol_id) {
   if(symbol_id >= buf.size()) buf.resize(buf.size() + 1000);
   return buf[symbol_id]; 
}

void MemTable::print() {
   std::println("\nMemTable\n");
   
   if(buf.empty()) {
      std::println("Empty TickerBuffer");
      return;
   }

   for(const auto& [n, block] : std::views::enumerate(buf)) {
      if(block.chunks.empty()) continue;

      std::println("Last ts={}\nLast price={}\nLast vol={}\n",
                  block.last_ts, block.last_price, block.last_vol);

      for(const auto& chunk : block.chunks) {
         std::println("Number of elements: {}", chunk.idx);

         for (size_t j = 0; j < chunk.idx; j++) {
               std::println("ts={} price={} vol={}",
                           chunk.ts[j], chunk.price[j], chunk.vol[j]);
         }
         std::println();
      }
   }
}
