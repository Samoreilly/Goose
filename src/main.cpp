
#include <format>
#include <print>

#include "Ingestion.h"
#include "wal/Wal.h"
#include "dsa/MemTable.h"
#include "dsa/MovingAverage.h"
#include "wal/WriteStaleWal.h"

int main() {

   std::println("GooseTSDB started");

   MemTable mem_table;
   
   Wal wal;
   WriteStateWal wsl{wal};
   wsl.move_wal();

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
