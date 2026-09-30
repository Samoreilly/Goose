
#include <print>

#include "dsa/RingBuffer.h"
#include "ingestion.h"
#include "wal/wal.h"


int main() {

   std::println("GooseTSDB started");

   // Ingestion<int, 65536> ingestor;
   // ingestor.start();


   Wal wal;
   std::println("{}", sizeof(Tick));

   


   return 0;
}
