
#include <chrono>
#include <cstdlib>
#include <print>
#include "Wal.h"
#include <filesystem>
#include <fstream>
#include <iostream>
#include <fcntl.h>
#include <thread>
#include <unistd.h>


namespace fs = std::filesystem;

//   4     24
// [crc][payload]
void Wal::append(const Tick& data) {



   const size_t S = sizeof(Tick);
  
   if(used + 4 + S > BUFFER_SIZE_B) {
      if(!flush()) {
         std::println(std::cerr, "Flush failed. Terminating to prevent further data loss");
         std::exit(EXIT_FAILURE);
      }

      //std::println("Clearing buffer and writing to disk");
   }

   uint8_t* p = buffer + used;
 
   //on replay, compare stored crc with the recomputed crc of the payload
   const uint32_t crc_payload = crc32(crc, reinterpret_cast<const uint8_t*>(&data), S);
   memcpy(p, &crc_payload, 4);
   p += 4;

   memcpy(p, &data, S);

   used += 4 + S;

   //std::println("Added to buffer");
}

void Wal::remove_stale() {

   for(const auto& entry : fs::directory_iterator(folder_name)) {
      
      auto name = entry.path().stem().native(); 
   
      size_t file_name {0};
      std::ignore = std::from_chars(name.data(), name.data() + name.size(), file_name);

      if(file_name < file_counter) {
         fs::remove(entry);
      }
   }


}

bool Wal::rotate() {
   if(!flush()) return false;
   if(fd == -1) return true;

   int rc = close(fd);
   fd = -1;
   
   current_size = 0;
   file_counter++;
   
   return rc == 0;
}

bool Wal::flush() {

   if(used == 0) return true;

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
      return false;
   }

   //Data potentially not safe so return false;
   if(fsync(fd) != 0) {
      return false;
   }

   current_size += used;
   used = 0;

   //std::println("Flushed");
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

void Wal::load_file_counter() {

   size_t file_counter {0};

   for(const auto& entry : fs::directory_iterator(folder_name)) { 
      
      //gives the files name excluding the extension
      auto file_name_wext = entry.path().stem().native();
      std::string_view file_name {file_name_wext};

      size_t file_number {0};
      //int to string
      auto [ptr, ec] = std::from_chars(file_name.data(), file_name.data() + file_name.size(), file_number);
      
      //not one of our files
      if(ec != std::errc{}) continue;

      if(file_number >= file_counter) {
         file_counter = file_number;
      }
   }

   this->file_counter = file_counter + 1; 


}





