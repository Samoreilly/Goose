
#include <cstdint>
#include <string.h>
#include <stddef.h>

#include "MemTable.h"


void MemTable::append(Tick& t) {
   TickerBuffer& slot = get(t.symbol_id);

   Chunk* chunk = &slot.chunks.back();
   if(chunk->idx == (chunk->N)) [[unlikely]] {
      slot.chunks.emplace_back();
      chunk = &slot.chunks.back();
   }
   
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
}

TickerBuffer& MemTable::get(uint32_t symbol_id) {
   return buf[symbol_id]; 
}
