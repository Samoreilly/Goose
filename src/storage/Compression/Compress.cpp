
#include <fcntl.h>
#include <filesystem>
#include <span>
#include <print>
#include <cstring>
#include "Compress.h"

namespace fs = std::filesystem;

void compression::compress(SSTable *sstable) {

   //fs::path file_path = folder_name + std::to_string(file_counter);
   //int fd = open(file_path.c_str(), O_CREAT | O_APPEND | O_WRONLY);

   //will use to mark start of each block
   size_t used {0};
   std::vector<int64_t> timestamp(sstable->blocks.size() * BLOCK_SIZE * 3);
   std::vector<int64_t> price(sstable->blocks.size() * BLOCK_SIZE * 3);

   for(auto& block : sstable->blocks) {
      //std span of block.elements in size
      compression::encode_ts_price(std::span<uint64_t>(block.ts).first(block.elements), timestamp, used); 
      compression::encode_ts_price(std::span<uint64_t>(block.price).first(block.elements), price, used);

   }

   BitWriter writer;

}

void compression::encode_ts_price(std::span<const uint64_t> data, std::vector<int64_t>& arr, size_t& used) {
   
   if(data.size() == 1) {
      arr[0] = data[0];
      return;
   }

   used = 0;

   //delta of delta
   for(size_t i {2};i < data.size(); i++) {
      arr[i] = (data[i] - data[i - 1]) - (data[i - 1] - data[i - 2]);
   }

   arr[0] = data[0];//first timestamp needed to calc subsequent timestamps 
   arr[1] = data[1] - data[0];//first delta

}

void compression::write_price(BitWriter& writer, const std::span<const int64_t> dods) {
  
   const int size = dods.size() + (12 * sizeof(uint8_t));
   std::vector<uint8_t> compressed(size);
   
   uint8_t* ptr = compressed.data();

   //copies timestamp and delta in 1st 12bytes
   memcpy(ptr, &dods[0], 8);
   ptr += 8;
   
   int64_t delta = static_cast<int32_t>(dods[1]);
   memcpy(ptr, &delta, 8);
   ptr += 8;

   auto zigzag = [](int val, int bits) {
      return ((val << 1) ^ (val >> bits));
   };
  
   //start at two to skip header
   for(size_t i {2};i < dods.size();i++) {
      const int64_t dod = dods[i];

      /*
         This is the control prefix, it tells us how much bits to write based off our value
         When compressed data is read, we will scan for a control prefix,
         if for example, we find '10' we know to read 7 bits
      */
      if(dod == 0) {
         writer.write(0b0, 1);
      }else if(dod > -64 && dod < 64) {
         writer.write(0b10, 2);
         int zz = zigzag(dod, 7);
         writer.write(zz, 7);
      }else if(dod > -256 && dod < 256) {
         writer.write(0b110, 3);
         int zz = zigzag(dod, 9);
         writer.write(zz, 9);     
      }else if(dod > -1024 && dod < 1024) {
         writer.write(0b1110, 4);
         int zz = zigzag(dod, 11);
         writer.write(zz, 11);
      }else {
         writer.write(0b1111, 4);
         int zz = zigzag(dod, 32);
         writer.write(zz, 32);
      }

   }

}


void compression::write_ts(BitWriter& writer, const std::span<const int64_t> dods) {
  
   const int size = dods.size() + (12 * sizeof(uint8_t));
   std::vector<uint8_t> compressed(size);
   
   uint8_t* ptr = compressed.data();

   //copies timestamp and delta in 1st 12bytes
   memcpy(ptr, &dods[0], 8);
   ptr += 8;
   
   int64_t delta = static_cast<int32_t>(dods[1]);
   memcpy(ptr, &delta, 8);
   ptr += 8;

  
   //start at two to skip header
   for(size_t i {2};i < dods.size();i++) {
      const int64_t dod = dods[i];

      /*
         This is the control prefix, it tells us how much bits to write based off our value
         When compressed data is read, we will scan for a control prefix,
         if for example, we find '10' we know to read 7 bits
      */
      if(dod == 0) {
         writer.write(0b0, 1);
      }else if(dod > -64 && dod < 64) {
         writer.write(0b10, 2);
         writer.write(dod, 7);
      }else if(dod > -256 && dod < 256) {
         writer.write(0b110, 3);
         writer.write(dod, 9);     
      }else if(dod > -1024 && dod < 1024) {
         writer.write(0b1110, 3);
         writer.write(dod, 11);
      }else {
         writer.write(0b1111, 4);
         writer.write(dod, 14);
      }

   }

}




