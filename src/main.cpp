
#include <format>
#include <print>

#include "ingestion.h"
#include "wal/wal.h"
#include "dsa/MemTable.h"
#include "dsa/MovingAverage.h"

int main() {

   std::println("GooseTSDB started");

   MemTable mem_table;
   Wal wal;
   

   // Ingestion<65536> ingestor{wal, mem_table};
   // ingestor.start();

   // mem_table.print();

   MovingAverage<30> thirty;
   
   //int count {0};
   for(int i {0};i < 30;i++) {
      thirty.add(i);
      
   }

   std::println("Moving average over 30 days = {}", thirty.get());

   return 0;
}
