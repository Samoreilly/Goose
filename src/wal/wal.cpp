
#include <print>
#include "wal.h"
#include <filesystem>
#include <fstream>
#include <iostream>
#include <fcntl.h>
#include <unistd.h>



//   4     24
// [crc][payload]
void Wal::append(const Tick& data) {

   const size_t S = sizeof(Tick);
   uint8_t* p = buffer;
   
   if(used + 4 + S > BUFFER_SIZE_B) {
      if(!flush()) {
         std::println("Flush failed");
      }

      std::println("Clearing buffer and writing to disk");
      return; 
   }

   memcpy(p + 4, &data, S);

   const uint32_t crc_payload = crc32(crc, p + 8, S);
   memcpy(p + 0, &crc_payload, 4);

   used += 4 + S;

   std::println("Added to buffer");
}


bool Wal::flush() {

   if(used == 0) return true;

   //If [file_counter].wal has no space, flush and
   if(fd != -1 && current_size > 0 && current_size + used > MAX_FILE_SIZE_B) {
      if(fsync(fd) != 0) return false;

      int rc = close(fd);
      fd = -1;
      current_size = 0;
      file_counter++;
      
      if(rc != 0) return false;
   }

   std::filesystem::path file_path = folder_name + std::to_string(file_counter) + ".wal";  

   if(fd == -1) {
      fd = open(file_path.c_str(), O_WRONLY | O_APPEND | O_CREAT, 0644);
      if(fd == -1) return false;

      if(!fsync_dir(folder_name.c_str())) {
         close(fd);
         fd = -1;
         return false;
      }
   }

  
   if(!write_bytes(fd, buffer, used)) {    
      std::println("Failed to write bytes to {}", file_path.c_str());
   }

   //Data potentially not safe so return false;
   if(fsync(fd) != 0) {
      return false;
   }

   current_size += used;
   used = 0;

   std::println("Flushed");
   return true;
}


//checks for error during writing to file
//prevents corrupted bytes/data
bool Wal::write_bytes(int fd, uint8_t buffer[], size_t count) {

    uint8_t* ptr = buffer;
    size_t remaining = count;

    while (remaining > 0) {

      ssize_t written = write(fd, ptr, remaining);
        
        if (written < 0) {
            if (errno == EINTR) {
                continue; //error writing, retry
            }
            return false;
        }
        
        ptr += written;
        remaining -= written;
    }
    
    return true;


}

bool Wal::fsync_dir(std::filesystem::path file_path) {
    int dfd = open(file_path.c_str(), O_RDONLY | O_DIRECTORY);
    if (dfd == -1) {
        return false;
    }

    bool ok = (fsync(dfd) == 0);
    close(dfd);
    return ok;
}




