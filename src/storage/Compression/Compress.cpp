
#include <cstdio>
#include <fcntl.h>
#include <unistd.h>
#include <cerrno>
#include <format>
#include <filesystem>
#include <span>
#include <array>
#include <print>
#include <cstring>
#include <fstream>
#include <fstream>
#include <sys/types.h>
#include <sys/stat.h>

#include "Compress.h"

namespace fs = std::filesystem;

void compression::compress(Storage& storage, SSTable *sstable) {

   //only the flusher calls this so the counter is safe to bump
   const size_t file_number = storage.file_counter++;

   //used to calc offsets as we go
   size_t base_offset {0};


   std::vector<int64_t> timestamp(sstable->blocks.size() * BLOCK_SIZE * 3);
   std::vector<int64_t> price(sstable->blocks.size() * BLOCK_SIZE * 3);
   std::vector<int32_t> volume(sstable->blocks.size() * BLOCK_SIZE * 3);

   BitWriter writer;

   for(auto& [symbol_id, meta] : sstable->file_metadata) {
      meta.file_number = file_number;

      for(BlockMeta& bmeta : meta.block_data) {
         
         Block& block = sstable->blocks[bmeta.block_id];
         bmeta.elements = block.elements;

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
   
   std::vector<uint64_t> out;

   //encodes full index at the end of file and 8 byte offset to start of offset
   encode_index(out, sstable->file_metadata, base_offset);

   //all data is fixed size until read
   for(auto& byte : out) {
      writer.write(byte, 64);
   }

   //crash safety
   //written to .tmp first then renamed, so a crash mid write never leaves a half written .goose file
   //this could corrupt data
   const std::string tmp_path = std::format("{}{:06}.tmp", storage.folder_name, file_number);
   const std::string file_path = std::format("{}{:06}.goose", storage.folder_name, file_number);

   int fd = open(tmp_path.c_str(), O_CREAT | O_TRUNC | O_WRONLY, 0644);
   if(fd == -1) {
      std::println("Failed to open {}", tmp_path);
      return;
   }

   size_t written {0};
   while(written < writer.out.size()) {
      ssize_t n = write(fd, writer.out.data() + written, writer.out.size() - written);
      if(n == -1) {
         if(errno == EINTR) continue;
         std::println("Failed to write bytes to {}", tmp_path);
         close(fd);
         return;
      }
      written += n;
   }

   if(fsync(fd) != 0 || close(fd) != 0) {
      std::println("Failed to sync {}", tmp_path);
      return;
   }

   fs::rename(tmp_path, file_path);

}

void compression::encode_ts_price(std::span<const uint64_t> data, std::vector<int64_t>& arr, size_t& used) {
   
   arr[0] = data[0];//first timestamp needed to calc subsequent timestamps 
   
   if(data.size() == 1) {
     return;
   }

   used = 0;

   //delta of delta
   for(size_t i {2};i < data.size(); i++) {
      arr[i] = (data[i] - data[i - 1]) - (data[i - 1] - data[i - 2]);
   }

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
   //delta or delta
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
      //arithmetic right shift to make negative/non negative btis fill with 1, 0 respectively
      return (static_cast<uint32_t>(val) << 1) ^ static_cast<uint32_t>(val >> 31);
   };
  
   //start at two to skip header
   for(size_t i {2};i < dods.size();i++) {
      const int32_t dod = dods[i];

      /*
         This is the control prefix, it tells us how much bits to write based off our value
         When compressed data is read, we will scan for a control prefix,
         if for example, we find '01' we know to read 7 bits
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

void compression::read_ts_price_vol(BitReader& reader, std::span<int64_t> dods) {

   if(dods.empty()) return;

   dods[0] = static_cast<int64_t>(reader.read(64));
   if(dods.size() == 1) return;
   dods[1] = static_cast<int64_t>(reader.read(64));

   auto unzigzag = [](uint64_t val) -> int64_t {
      return static_cast<int64_t>((val >> 1) ^ -(val & 1));
   };

   //bits to read for each prefix, indexed by how many 1s it has
   //0 -> 0, 10 -> 7, 110 -> 9, 1110 -> 11, 1111 -> 64
   constexpr std::array<uint32_t, 5> widths {0, 7, 9, 11, 64};

   for(size_t i {2};i < dods.size();i++) {

      //count 1s until a 0, the longest prefix is four 1s
      uint32_t ones {0};
      while(ones < 4 && reader.read(1) == 1) {
         ones++;
      }

      //width 0 reads nothing so a dod of 0 comes out as 0
      dods[i] = unzigzag(reader.read(widths[ones]));
   }

}

void compression::read_vol(BitReader& reader, std::span<int32_t> dods) {

   if(dods.empty()) return;

   dods[0] = static_cast<int32_t>(reader.read(32));
   if(dods.size() == 1) return;
   dods[1] = static_cast<int32_t>(reader.read(32));

   //https://gist.github.com/mfuerstenau/ba870a29e16536fdbaba
   auto unzigzag = [](uint32_t val) -> int32_t {
      return static_cast<int32_t>((val >> 1) ^ -(val & 1));
   };

   //this undoes write_vol() and write_ts_price_vol 
   //same as read_ts_price_vol but the last width is 32
   constexpr std::array<uint32_t, 5> widths {0, 7, 9, 11, 32};

   //checks how much ones then reads mapped amount based of ones
   for(size_t i {2};i < dods.size();i++) {

      uint32_t ones {0};
      while(ones < 4 && reader.read(1) == 1) {
         ones++;
      }

      dods[i] = unzigzag(static_cast<uint32_t>(reader.read(widths[ones])));
   }

}

void compression::decode_ts_price(std::span<const int64_t> arr, std::span<uint64_t> data) {

   if(arr.empty()) return;

   data[0] = arr[0];
   if(arr.size() == 1) return;

   //delta is rebuilt as we go, then added onto the previous value
   uint64_t delta = arr[1];
   data[1] = data[0] + delta;

   for(size_t i {2};i < arr.size();i++) {
      delta += arr[i];
      data[i] = data[i - 1] + delta;
   }

}

void compression::decode_vol(std::span<const int32_t> arr, std::span<uint32_t> data) {

   if(arr.empty()) return;

   data[0] = arr[0];

   for(size_t i {1};i < arr.size();i++) {
      data[i] = data[i - 1] + arr[i];
   }

}

Block compression::decode(Storage& storage, SSTable* sstable, uint64_t symbol_id, uint64_t block_id) {

   Block block;

   auto s_data = sstable->file_metadata.find(symbol_id);
   if(s_data == sstable->file_metadata.end()) return block;

   const std::string file_name = std::format("{}{:06}.goose", storage.folder_name, s_data->second.file_number);
   std::ifstream file(file_name, std::ios::binary);

   if(!file) {
      std::println("No file found");
      return block;
   }

   //find the metadata for block_id
   const BlockMeta* bmeta {nullptr};
   for(const BlockMeta& b : s_data->second.block_data) {
      if(b.block_id == block_id) {
         bmeta = &b;
         break;
      }
   }
   if(!bmeta) return block;

   block.elements = bmeta->elements;

   auto read_bytes = [&](size_t offset, size_t size) {
      std::vector<uint8_t> bytes(size);
      file.seekg(offset, std::ios::beg);
      file.read(reinterpret_cast<char*>(bytes.data()), size);
      return bytes;
   };

   //decodes ts, price and vol seperately
   std::vector<int64_t> dods(block.elements);
   std::vector<int32_t> vol_deltas(block.elements);

   std::vector<uint8_t> bytes = read_bytes(bmeta->ts_offset, bmeta->ts_size);
   BitReader ts_reader {bytes};
   compression::read_ts_price_vol(ts_reader, dods);
   compression::decode_ts_price(dods, std::span(block.ts).first(block.elements));

   bytes = read_bytes(bmeta->price_offset, bmeta->price_size);
   BitReader price_reader {bytes};
   compression::read_ts_price_vol(price_reader, dods);
   compression::decode_ts_price(dods, std::span(block.price).first(block.elements));

   bytes = read_bytes(bmeta->vol_offset, bmeta->vol_size);
   BitReader vol_reader {bytes};
   compression::read_vol(vol_reader, vol_deltas);
   compression::decode_vol(vol_deltas, std::span(block.vol).first(block.elements));

   return block;

}



void compression::encode_index(std::vector<uint64_t>& out, const std::unordered_map<uint64_t, FileMetadata>& metadata, size_t base_offset) {

   //the base offset will be stored in the last 8 bytes of the file

   //values will be stored as 8 bytes but read back as actual type
   //metadata will be stored at the end
   for(const auto& [key, val] : metadata) {

      out.push_back(key);//symbol_id

      out.push_back(val.file_number);
      out.push_back(val.max_ts);
      out.push_back(val.min_ts);
      out.push_back(val.max_price);
      out.push_back(val.min_price);
      out.push_back(val.block_offset);

      out.push_back(val.block_data.size());
     
      //metadata last
      for(const auto& bmeta : val.block_data) {
         
         out.push_back(static_cast<uint64_t>(bmeta.block_id));
         
         out.push_back(static_cast<uint64_t>(bmeta.bmax_ts));
         out.push_back(static_cast<uint64_t>(bmeta.bmin_ts));
         out.push_back(static_cast<uint64_t>(bmeta.bmax_price));
         out.push_back(static_cast<uint64_t>(bmeta.bmin_price));
         
         out.push_back(static_cast<uint64_t>(bmeta.elements));

         out.push_back(static_cast<uint64_t>(bmeta.ts_offset));
         out.push_back(static_cast<uint64_t>(bmeta.ts_size));
         out.push_back(static_cast<uint64_t>(bmeta.price_offset));
         out.push_back(static_cast<uint64_t>(bmeta.price_size));
         out.push_back(static_cast<uint64_t>(bmeta.vol_offset));
         out.push_back(static_cast<uint64_t>(bmeta.vol_size));
      }

   }

   //this is the end of the file, it will be read to know
   //the starting point of the index
   out.push_back(static_cast<uint64_t>(base_offset));
}

// BLOCK 1 | BLOCK 2 | INDEX | 8 byte integer storing offset to index
std::unordered_map<uint64_t, FileMetadata> compression::decode_index(Storage& storage, const uint64_t file_counter) {

   const std::string file_path = std::format("{}{:06}.goose", storage.folder_name, file_counter);
   int fd = open(file_path.c_str(), O_RDONLY, 0644);

   uint64_t file_size {0};
   struct stat st;

   if(!fstat(fd, &st)) {
      file_size = st.st_size;
   }

   uint64_t index_offset {0};
   //reads last 8 bytes of file
   pread(fd, &index_offset, 8, file_size - 8);


   const size_t index_size = file_size - 8 - index_offset;

   std::vector<uint64_t> in(index_size / 8);

   std::unordered_map<uint64_t, FileMetadata> metadata;
   //read full index into buffer
   pread(fd, in.data(), index_size, index_offset);

   //same order as encode_index, i moves along as each value is read
   for(size_t i {0};i < in.size();) {

      uint64_t symbol_id = in[i++];
      FileMetadata& meta = metadata[symbol_id];

      meta.file_number = in[i++];
      meta.max_ts = in[i++];
      meta.min_ts = in[i++];
      meta.max_price = in[i++];
      meta.min_price = in[i++];
      meta.block_offset = in[i++];

      const uint64_t n_blocks = in[i++];
      meta.block_data.resize(n_blocks);

      for(BlockMeta& bmeta : meta.block_data) {

         bmeta.block_id = in[i++];

         bmeta.bmax_ts = in[i++];
         bmeta.bmin_ts = in[i++];
         bmeta.bmax_price = in[i++];
         bmeta.bmin_price = in[i++];

         bmeta.elements = static_cast<uint32_t>(in[i++]);

         bmeta.ts_offset = in[i++];
         bmeta.ts_size = in[i++];
         bmeta.price_offset = in[i++];
         bmeta.price_size = in[i++];
         bmeta.vol_offset = in[i++];
         bmeta.vol_size = in[i++];
      }

   }

   close(fd);

   return metadata;

}















