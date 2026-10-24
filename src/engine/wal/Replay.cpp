
#include <fcntl.h>
#include <filesystem>
#include <zlib.h>
#include <sys/stat.h>
#include <vector>
#include <print>
#include <unistd.h>
#include <cstring>

#include "Replay.h"
#include "../DataTypes.h"
#include "../dsa/MemTable.h"

namespace fs = std::filesystem;


void Replay::stream(MemTable& mem_table) {

   size_t total_size {0};

   for(const auto& entry : fs::directory_iterator(folder_name)) {
      total_size += entry.file_size();
   }

   std::vector<Tick> stream;
   //each tick is sizeof(Tick) + 4 for crc payload
   //this gives us perfect vector size
   stream.reserve(total_size / (sizeof(Tick) + 4));
   
   for(const auto& entry : fs::directory_iterator(folder_name)) {
      if(!read(entry, stream)) continue;
   }

   //not logged to the WAL again, the ticks are already in the old WAL files
   for(auto& tick : stream) {
      mem_table.append(tick, false);
   }

   std::println("Replayed {} ticks", stream.size());
}


bool Replay::read(const auto& file_entry, auto& stream) {

   int fd = open(file_entry.path().c_str(), O_RDONLY, std::ios::binary);
 
   if(fd == -1) {
      std::println("Unable to replay File: {}", file_entry.path().stem().native());
      return false;
   }

   struct stat st;
   size_t file_size {0};
   if(!fstat(fd, &st)) {
      file_size = st.st_size;
   }

   std::vector<uint8_t> buffer;
   buffer.resize(file_size);

   pread(fd, buffer.data(), file_size, 0);

   uint8_t* p = buffer.data();
   const size_t S = sizeof(Tick);

   size_t used {0};
   while(used != file_size) {
      
      uint32_t old_crc {0};
      //computed crc from wal append
      memcpy(&old_crc, p, 4);

      Tick t{};
      memcpy(&t, p + 4, sizeof(Tick));
      
      //compute crc32 for Tick
      const uint32_t tick_crc = crc32(crc, reinterpret_cast<uint8_t*>(&t), S);
      
      //compare old crc with newly computed crc to check for data corruption
      if(old_crc != tick_crc) {
         //whole file is most likely corrupted
         //so skip
         return false;
      }

      //sizeof(Tick) + crc32 payload
      p += S + 4;
      used += S + 4;
      stream.emplace_back(t);
   
   }

   close(fd);

   return true;
}
