
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

   //used to calc offsets as we go
   size_t base_offset {0};


   std::vector<int64_t> timestamp(sstable->blocks.size() * BLOCK_SIZE * 3);
   std::vector<int64_t> price(sstable->blocks.size() * BLOCK_SIZE * 3);
   std::vector<int32_t> volume(sstable->blocks.size() * BLOCK_SIZE * 3);

   BitWriter writer;

   for(auto& [symbol_id, meta] : sstable->file_metadata) {
      for(BlockMeta& bmeta : meta.block_data) {
         
         Block& block = sstable->blocks[bmeta.block_id];

         bmeta.ts_offset = writer.out.size();
         compression::encode_ts_price(std::span<uint64_t>(block.ts).first(block.elements), timestamp, base_offset); 
         compression::write_ts_price_vol(writer, std::span(timestamp).first(block.elements));
         writer.flush();

         bmeta.ts_size = (writer.out.size() - bmeta.ts_offset);
         base_offset += writer.out.size();
         
         bmeta.price_offset = writer.out.size(); 
         compression::encode_ts_price(std::span<uint64_t>(block.price).first(block.elements), price, base_offset);
         compression::write_ts_price_vol(writer, std::span(price).first(block.elements));
         writer.flush();
         
         bmeta.price_size = (writer.out.size() - bmeta.price_offset);
         base_offset += writer.out.size();
         
         bmeta.vol_offset = writer.out.size();
         compression::encode_vol(std::span<uint32_t>(block.vol).first(block.elements), volume, base_offset);
         compression::write_vol(writer, std::span(volume).first(block.elements));
         //clears extra bits after each column is done
         writer.flush();
         
         bmeta.vol_size = (writer.out.size() - bmeta.vol_offset);
         base_offset += writer.out.size();
         
      }
   }
   
   std::vector<uint8_t> data = writer.out;

}

void compression::encode_ts_price(std::span<const uint64_t> data, std::vector<int64_t>& arr, size_t& used) {
   
   arr[0] = data[0];
   
   if(data.size() == 1) {
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

void compression::encode_vol(std::span<const uint32_t> data, std::vector<int32_t>& arr, size_t& used) {
   
   used = 0;
   arr[0] = data[0];
   
   if(data.size() == 1) {
      return;
   }

   for(size_t i {1};i < data.size();i++) {
      arr[i] = data[i] - data[i - 1];
   }

}

void compression::write_ts_price_vol(BitWriter& writer, const std::span<const int64_t> dods) {

   if(dods.empty()) return;

   //single element blocks still need their first value
   writer.write(dods[0], 64);
   if(dods.size() == 1) return;
   writer.write(dods[1], 64);

   //https://gist.github.com/mfuerstenau/ba870a29e16536fdbaba
   //>> 63 smears the sign bit across all 64 bits
   auto zigzag = [](int64_t val) -> uint64_t {
      return (static_cast<uint64_t>(val) << 1) ^ static_cast<uint64_t>(val >> 63);
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
         writer.write(0b01, 2);
         writer.write(zigzag(dod), 7);
      }else if(dod > -256 && dod < 256) {
         writer.write(0b011, 3);
         writer.write(zigzag(dod), 9);
      }else if(dod > -1024 && dod < 1024) {
         writer.write(0b0111, 4);
         writer.write(zigzag(dod), 11);
      }else {
         //64 bits so large dods (e.g. ns timestamps) aren't truncated
         writer.write(0b1111, 4);
         writer.write(zigzag(dod), 64);
      }

   }

}

void compression::write_vol(BitWriter& writer, const std::span<const int32_t> dods) {
  
  
   if(dods.empty()) return;

   //single element blocks still need their first value
   writer.write(static_cast<uint32_t>(dods[0]), 32);
   if(dods.size() == 1) return;
   writer.write(static_cast<uint32_t>(dods[1]), 32);

   //https://gist.github.com/mfuerstenau/ba870a29e16536fdbaba
   //>> 31 smears the sign bit across all 32 bits
   auto zigzag = [](int32_t val) -> uint32_t {
      return (static_cast<uint32_t>(val) << 1) ^ static_cast<uint32_t>(val >> 31);
   };
  
   //start at two to skip header
   for(size_t i {2};i < dods.size();i++) {
      const int32_t dod = dods[i];

      /*
         This is the control prefix, it tells us how much bits to write based off our value
         When compressed data is read, we will scan for a control prefix,
         if for example, we find '10' we know to read 7 bits
      */
      if(dod == 0) {
         writer.write(0b0, 1);
      }else if(dod > -64 && dod < 64) {
         writer.write(0b01, 2);
         writer.write(zigzag(dod), 7);
      }else if(dod > -256 && dod < 256) {
         writer.write(0b011, 3);
         writer.write(zigzag(dod), 9);
      }else if(dod > -1024 && dod < 1024) {
         writer.write(0b0111, 4);
         writer.write(zigzag(dod), 11);
      }else {
         writer.write(0b1111, 4);
         writer.write(zigzag(dod), 32);
      }

   }

}










