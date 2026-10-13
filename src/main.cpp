#include <format>
#include <print>
#include <thread>
#include <chrono>
#include <random>
#include <vector>

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

   constexpr int N_TICKS = 50000000;
   constexpr int N_SYMBOLS = 5000;

   std::mt19937_64 gen{42};

   //creates random symbols
   std::vector<double> weights(N_SYMBOLS);
   for(int k {0};k < N_SYMBOLS;k++) {
      weights[k] = 1.0 / (k + 1);
   }
   std::discrete_distribution<uint32_t> symbol(weights.begin(), weights.end());
   std::uniform_int_distribution<uint64_t> jitter(0, 999);

   auto dur = std::chrono::duration<double>(0);

   for(int i {0};i < 5;i++) {
      std::vector<Tick> ticks;
      ticks.reserve(N_TICKS);
      for(int j {0};j < N_TICKS;j++) {
         ticks.push_back({(uint64_t)(1000000 + j) + jitter(gen), 10000 + jitter(gen), (uint32_t)jitter(gen), symbol(gen)});
      }

   //will contain global indexes, will become std vector<SSTable>
   Storage storage;

      auto start = std::chrono::steady_clock::now();

      {
         MemTable mem_table{storage};

         for(auto& tick : ticks) {
            mem_table.append(tick);
         }
      }

      auto elapsed = std::chrono::steady_clock::now() - start;
      std::println("Time taken {}", elapsed);
      
      dur += std::chrono::duration<double>(elapsed);
   }


 
   std::println("{:.2f}M ticks/s", ((N_TICKS * 5) / dur.count()) / 1e6);
   // auto point = Sit::lookup(storage, 1, 1000000, 1100000);
   // std::println("Point lookup rows: {}", point.size());
   // for(const auto& r : point) {
   //    std::println("ts={} price={} vol={} sym={}", r.ts, r.price, r.vol, r.symbol_id);
   // }

   return 0;

}
