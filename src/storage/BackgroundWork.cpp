
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

   while(true) {

      //locks
      std::unique_lock<std::mutex> lk(mem.mu);
     
      //signalled from MemTable destructor
      if(mem.stop) [[unlikely]]{
         //flush remaining from buffer

         if(mem.total_ticks > 0) {
            std::vector<uint32_t> counts;
            counts.assign(mem.buf.size(), 0);

            uint32_t n_symbols = copy_chunks(mem, counts);
            lk.unlock();
            Sit s(std::move(mem.sorted), std::move(counts), n_symbols);
         }

         return;
      }

      const auto timepoint = mem.last_append + std::chrono::seconds(5);
   
      //unlocks, sleeps for 5 seconds
      //or until append() calls notify_one(), then checks condition, if true it locks
      bool condition_met = mem.cond_var.wait_until(lk, timepoint, [&mem] {
        return mem.total_ticks > 0 && (mem.total_ticks >= MemTable::MAX_TICKS || (std::chrono::steady_clock::now() - mem.last_append) >= std::chrono::seconds(5));
      });
      
      if(condition_met) {
         //counts amount of data per symbol. Later used to calc number of blocks needed.
         std::vector<uint32_t> counts;
         counts.assign(mem.buf.size(), 0);
         //swap buffer into sorted
         uint32_t n_symbols = copy_chunks(mem, counts);

         mem.reset_memtable();
         //resources are acquired so we unlock asap, so append() can start again  
         lk.unlock();
 
         Sit s(std::move(mem.sorted), std::move(counts), n_symbols);  
         
     
      }


   
   }
}
