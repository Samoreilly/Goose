
#include <format>
#include <print>

#include "ingestion.h"
#include "wal/wal.h"


int main() {

   std::println("GooseTSDB started");

   Wal wal;


   Ingestion<65536> ingestor{wal};

   Tick t {1, 2, 3, 4};

   wal.append(t);
   wal.append(t);
   wal.append(t);
   wal.append(t);

   


   return 0;
}
