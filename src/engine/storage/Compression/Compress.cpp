
#include <cstdio>
#include <fcntl.h>
#include <unistd.h>
#include <cerrno>
#include <format>
#include <filesystem>
#include <span>
#include <array>
#include <print>
#include <mutex>
#include <cstring>
#include <fstream>
#include <fstream>
#include <sys/types.h>
#include <sys/stat.h>

#include "Compress.h"

namespace fs = std::filesystem;

bool compression::compress(Storage& storage, SSTable *sstable) {
 
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
   const std::string tmp_path = std::format("{}{:06}.tmp", storage.goose_folder_name, file_number);
   const std::string file_path = std::format("{}{:06}.goose", storage.goose_folder_name, file_number);

   int fd = open(tmp_path.c_str(), O_CREAT | O_TRUNC | O_WRONLY, 0644);
   if(fd == -1) {
      std::println("Failed to open {}", tmp_path);
      return false;
   }

   size_t written {0};
   while(written < writer.out.size()) {
      ssize_t n = write(fd, writer.out.data() + written, writer.out.size() - written);
      if(n == -1) {
         if(errno == EINTR) continue;
         std::println("Failed to write bytes to {}", tmp_path);
         close(fd);
         return false;
      }
      written += n;
   }

   if(fsync(fd) != 0 || close(fd) != 0) {
      std::println("Failed to sync {}", tmp_path);
      return false;
   }

   fs::rename(tmp_path, file_path);
   fsync(fd);

   return true;
}

Block compression::decode(Storage& storage, SSTable* sstable, uint64_t symbol_id, uint64_t block_id) {

   Block block;

   auto s_data = sstable->file_metadata.find(symbol_id);
   if(s_data == sstable->file_metadata.end()) return block;

   const std::string file_name = std::format("{}{:06}.goose", storage.goose_folder_name, s_data->second.file_number);
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

   //values will be stored at a fixed width of 8 bytes but read back there actual size
   //index will be stored at the end with a offset to the index in the last 8 bytes

   out.push_back(static_cast<uint64_t>(0));//compaction level

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
   out.push_back(static_cast<uint64_t>(base_offset) + 8);
}

// BLOCK 1 | BLOCK 2 | INDEX | 8 byte integer storing offset to index
std::unordered_map<uint64_t, FileMetadata> compression::decode_index(Storage& storage, const uint64_t file_counter) {

   const std::string file_path = std::format("{}{:06}.goose", storage.goose_folder_name, file_counter);
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
  
   //read full index into buffer
   pread(fd, in.data(), index_size, index_offset);

   fs::path path(file_path);

   auto metadata = compression::get_index(path);

   close(fd);

   return metadata;

}


std::unordered_map<uint64_t, FileMetadata> compression::get_index(std::filesystem::path file_path) {

   std::unordered_map<uint64_t, FileMetadata> metadata;

   //file may have been compacted away since it was listed, empty index means unreadable
   int fd = open(file_path.c_str(), O_RDONLY, 0644);
   if(fd == -1) return metadata;

   struct stat st;
   if(fstat(fd, &st) != 0 || st.st_size < 8) {
      close(fd);
      return metadata;
   }

   const uint64_t file_size = st.st_size;

   uint64_t index_offset {0};
   //reads last 8 bytes of file
   if(pread(fd, &index_offset, 8, file_size - 8) != 8 || index_offset > file_size - 8) {
      close(fd);
      return metadata;
   }

   const size_t index_size = file_size - 8 - index_offset;

   std::vector<uint64_t> in(index_size / 8);


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


std::unique_ptr<SSTable> compression::load_sstable(std::filesystem::path file_path) {

   //sstable holds a mutex so it cant be returned by value
   auto sstable = std::make_unique<SSTable>();
   sstable->file_metadata = compression::get_index(file_path);
   if(sstable->file_metadata.empty()) return nullptr;

   //whole file is read once, columns are decoded from it
   std::ifstream file(file_path, std::ios::binary);
   std::error_code ec;
   const auto size = fs::file_size(file_path, ec);
   if(!file || ec) {
      std::println("No file found");
      return nullptr;
   }
   std::vector<uint8_t> bytes(size);
   file.read(reinterpret_cast<char*>(bytes.data()), bytes.size());

   //block_id indexes into blocks
   uint64_t max_id {0};
   for(const auto& [symbol_id, meta] : sstable->file_metadata) {
      for(const BlockMeta& bmeta : meta.block_data) {
         max_id = std::max(max_id, bmeta.block_id);
      }
   }
   if(!sstable->file_metadata.empty()) sstable->blocks.resize(max_id + 1);

   std::vector<int64_t> dods(BLOCK_SIZE);
   std::vector<int32_t> vol_deltas(BLOCK_SIZE);

   for(const auto& [symbol_id, meta] : sstable->file_metadata) {
      for(const BlockMeta& bmeta : meta.block_data) {

         Block& block = sstable->blocks[bmeta.block_id];
         block.elements = bmeta.elements;

         auto block_dods = std::span(dods).first(block.elements);
         auto block_vol_deltas = std::span(vol_deltas).first(block.elements);

         BitReader ts_reader {std::span<const uint8_t>(bytes).subspan(bmeta.ts_offset, bmeta.ts_size)};
         compression::read_ts_price_vol(ts_reader, block_dods);
         compression::decode_ts_price(block_dods, std::span(block.ts).first(block.elements));

         BitReader price_reader {std::span<const uint8_t>(bytes).subspan(bmeta.price_offset, bmeta.price_size)};
         compression::read_ts_price_vol(price_reader, block_dods);
         compression::decode_ts_price(block_dods, std::span(block.price).first(block.elements));

         BitReader vol_reader {std::span<const uint8_t>(bytes).subspan(bmeta.vol_offset, bmeta.vol_size)};
         compression::read_vol(vol_reader, block_vol_deltas);
         compression::decode_vol(block_vol_deltas, std::span(block.vol).first(block.elements));
      }
   }

   return sstable;

}
















