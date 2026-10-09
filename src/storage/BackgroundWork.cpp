
#include <thread>
#include <mutex>
#include <print>

#include "BackgroundWork.h"
#include "SSTable/Sit.h"

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
         Sit s(std::move(mem.sorted), mem);  
         mem.reset_memtable();
         //resources are acquired so we unlock asap, so append() can start again
         lk.unlock();
         
         std::println("Cleared {}", mem.total_ticks);
      }


   
   }
}
