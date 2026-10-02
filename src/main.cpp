
#include <format>
#include <print>

#include "ingestion.h"
#include "wal/wal.h"
#include "dsa/MemTable.h"

int main() {

   std::println("GooseTSDB started");

   MemTable mem_table;
   Wal wal;
   

   Ingestion<65536> ingestor{wal, mem_table};
   //ingestor.start();
    



   return 0;
}
