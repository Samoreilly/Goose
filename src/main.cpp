
#include <format>
#include <print>

#include "dsa/RingBuffer.h"
#include "ingestion.h"
#include "wal/wal.h"


int main() {

   std::println("GooseTSDB started");

   // Ingestion<int, 65536> ingestor;
   // ingestor.start();


   Wal wal;

   Tick t {1, 2, 3, 4};

   wal.append(t);
   wal.append(t);
   wal.append(t);
   wal.append(t);

   


   return 0;
}
