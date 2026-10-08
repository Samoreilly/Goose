
#include <format>
#include <print>

#include "Ingestion.h"
#include "wal/Wal.h"
#include "dsa/MemTable.h"
#include "dsa/MovingAverage.h"
#include "wal/WriteStaleWal.h"

#include <random>

int main() {

   std::println("GooseTSDB started");

   MemTable mem_table;
   
   std::random_device rd;

   std::mt19937 gen(rd());

   std::uniform_int_distribution<uint64_t> distrib64(10000000000ULL, 99999999999ULL);
   std::uniform_int_distribution<uint32_t> distrib32(100000, 999999);
   
   uint32_t symbol_id {0};
   for(int i {0};i < 15000;i++) {
      Tick tick{distrib64(gen), distrib64(gen), distrib32(gen), symbol_id++};
      mem_table.append(tick);
   }

   mem_table.sort();

   // Wal wal;
   // WriteStateWal wsl{wal};
   // wsl.move_wal();

   // Tick t{1, 2, 3, 4};
   // wal.append(t);   

   // Ingestion<65536> ingestor{wal, mem_table};
   // ingestor.start();

   // mem_table.print();

   // MovingAverage<30> thirty;
   // 
   // int count {0};
   // for(int i {0};i < 30;i++) {
   //    thirty.add(i);
   //    
   // }

   // std::println("Moving average over 30 days = {}", thirty.get());

   return 0;
}
