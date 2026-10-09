
#include <cstdint>
#include <string.h>
#include <stddef.h>
#include <ranges>
#include <print>
#include <mutex>

#include "MemTable.h"



void MemTable::append(Tick& t) {

   //Lock so background cleanup thread doesn't interfere
   std::unique_lock<std::mutex> lk(mu);

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

   total_ticks++;
   last_append = std::chrono::steady_clock::now();


   if(total_ticks >= MemTable::MAX_TICKS) [[unlikely]] {
      lk.unlock();
      cond_var.notify_one();
   }

}

TickerBuffer& MemTable::get(uint32_t symbol_id) {
   if(symbol_id >= buf.size()) [[unlikely]] buf.resize(symbol_id + 1000);
   
   return buf[symbol_id]; 
}

void MemTable::print() {
   std::println("\nMemTable\n");
   
   if(buf.empty()) [[unlikely]] {
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


void MemTable::print_chunksort(const std::vector<ChunkSort>& samples) {
    std::println("\n\nIndex\tTimestamp\tPrice\t\tVolume");

    for (size_t i = 0; i < samples.size(); ++i) {
        const auto& s = samples[i];
        std::println("{}\t{}\t{}\t\t{}", i, s.ts, s.price, s.vol);
    }

}


