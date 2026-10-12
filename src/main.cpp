
#include <emmintrin.h>
#include <format>
#include <print>
#include <thread>
#include <chrono>

#include "Ingestion.h"
#include "wal/Wal.h"
#include "dsa/MemTable.h"
#include "dsa/MovingAverage.h"
#include "wal/WriteStaleWal.h"
#include "storage/SSTable/SSTable.h"
#include "storage/SSTable/Sit.h"
#include "storage/Layout/Storage.h"


int main() {

   std::println("GooseTSDB started");

   //will contain global indexes, will become std vector<SSTable>
   Storage storage;
   MemTable mem_table{storage};

  constexpr int N_TICKS = 105000;
   for(int i {0};i < N_TICKS;i++) {
      Tick tick{(uint64_t)(1000000 + i), (uint64_t)(100 + i), (uint32_t)i, 1};
      mem_table.append(tick);
   }

   //sleep to wait for tick data to propagate across indexes
   while(storage.sstables.size() != 11) {
      _mm_pause();
  //    std::println("{}", storage.sstables.size());
   }

   // auto point = Sit::lookup(storage, 1, 1000000, 1100000);
   // std::println("Point lookup rows: {}", point.size());
   // for(const auto& r : point) {
   //    std::println("ts={} price={} vol={} sym={}", r.ts, r.price, r.vol, r.symbol_id);
   // }


   
   return 0;

}
