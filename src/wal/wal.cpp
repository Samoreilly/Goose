
#include <print>
#include "wal.h"
#include <filesystem>
#include <fstream>
#include <iostream>
#include <fcntl.h>



//   4     28
// [crc][payload]
void Wal::append(const Tick& data) {

   const size_t S = sizeof(Tick);
   uint8_t* p = &buffer[used];
   
   if(used + 4 + S > BUFFER_SIZE_B) {
      flush();
      std::println("Clearing buffer and writing to disk");
      return; 
   }

   memcpy(p + 4, &data, S);

   const uint32_t crc_payload = crc32(crc, p + 8, S);
   memcpy(p + 0, &crc_payload, 4);

   used += 4 + S;

   std::println("Added to buffer");
}


void Wal::flush() {

   std::filesystem::path file_path = folder_name + std::to_string(file_counter);
   std::ofstream f(file_path, std::ios::app);
   
   const size_t file_size = std::filesystem::file_size(file_path);
   
   if(file_size + BUFFER_SIZE_B > MAX_FILE_SIZE_B) {
      file_counter++;
      flush();//try again
      return;
   }

   int fd = open(file_path.c_str(), O_WRONLY);
   if(fd == -1) {}

   write(fd, buffer, BUFFER_SIZE_B);

   std::println("Flushed to {}", file_path.c_str());


}
