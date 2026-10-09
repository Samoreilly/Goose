
#include <thread>
#include <mutex>
#include <print>

#include "BackgroundWork.h"
#include "SSTable/Sit.h"


void copy_chunks(MemTable& mem) {
   mem.sorted.clear();
   mem.sorted.reserve(mem.total_ticks);

  for(const auto& ticker : mem.buf) {
   
      for(const auto& chunk : ticker.chunks) {
  
         for(size_t i {0}; i < chunk.idx;i++) {
            mem.sorted.push_back({chunk.ts[i], chunk.price[i], chunk.vol[i]});
         }
      }
   }
}

void cleanup_memtable(MemTable& mem) {

   while(true) {
      std::unique_lock<std::mutex> lk(mem.mu);
     
      //signalled from MemTable destructor
      if(mem.stop) [[unlikely]] return;

      const auto timepoint = mem.last_append + std::chrono::seconds(5);
   
      //sleeps for 5 seconds, then checks condition, if true it locks
      bool condition_met = mem.cond_var.wait_until(lk, timepoint, [&mem] {
         std::println("Checking condition");
         return mem.total_ticks > 0 && (mem.total_ticks >= MemTable::MAX_TICKS || (std::chrono::steady_clock::now() - mem.last_append) >= std::chrono::seconds(5));
      });

      if(condition_met) {
         
         std::println("Before cleared {}", mem.total_ticks);

         //swap buffer into sorted
         copy_chunks(mem);

         mem.reset_memtable();
         //resources are acquired so we unlock asap, so append() can start again  
         lk.unlock();
 
         Sit s(std::move(mem.sorted));  
            
       
         std::println("Cleared {}", mem.total_ticks);
      
      }


   
   }
}
