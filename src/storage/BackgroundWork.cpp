
#include <thread>
#include <mutex>
#include <print>
#include <unordered_set>

#include "BackgroundWork.h"
#include "SSTable/Sit.h"


/*
Moves buffer into mem.sorted, unsorted. Sort occurs later
*/

uint32_t copy_chunks(MemTable& mem, std::vector<uint32_t>& counts) {
   mem.sorted.clear();
   mem.sorted.reserve(mem.total_ticks);

   std::unordered_set<std::uint32_t> n_symbols;

   for(const auto& ticker : mem.buf) {
      const auto s_id = ticker.symbol_id;

      for(const auto& chunk : ticker.chunks) {
         counts[s_id] += chunk.idx;//counts data per symbol_id. used to calc number of blocks later

         for(size_t i {0}; i < chunk.idx;i++) {
            n_symbols.insert(s_id);//counts tickers
            mem.sorted.push_back({chunk.ts[i], chunk.price[i], chunk.vol[i], s_id});
         }
      }
   }
   
   return n_symbols.size();
}

void cleanup_memtable(MemTable& mem) {
   //locks
   std::unique_lock<std::mutex> lk(mem.mu);
 
    while(true) {

      //blocks thread and unlocks until mem.stop or a tick has been appended
      //unlock is needed to give append() access
      mem.cond_var.wait(lk, [&mem] { return mem.stop || mem.total_ticks > 0; });
    
      //loop until total_ticks >= max_ticks or 5s timeout
      while(!mem.stop && mem.total_ticks < MemTable::MAX_TICKS) {
         //blocks thread until time, timeout status occurs when thread has been idle for 5 seconds
         const auto status = mem.cond_var.wait_until(lk, mem.last_append + std::chrono::seconds(5));

         //if notified and reached time limit
         if(status == std::cv_status::timeout && ((std::chrono::steady_clock::now() - mem.last_append) >= std::chrono::seconds(5))) {
            break;
         }
      }

      //this comparison will only be triggered once total_ticks == max_ticks
      //or to flush leftovers in buffer from 5s timeout
      if(mem.total_ticks > 0) { 
         std::vector<uint32_t> counts(mem.buf.size(), 0);

         uint32_t n_symbols = copy_chunks(mem, counts);
         mem.reset_memtable();
         lk.unlock();
         Sit{mem.storage, std::move(mem.sorted), std::move(counts), n_symbols};
         lk.lock();
      }

      //drains unflushed rows before exiting as stop may arrive at any time,
      if(mem.stop && mem.total_ticks == 0) return;
   
   }
}
