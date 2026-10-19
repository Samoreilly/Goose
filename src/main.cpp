#include <format>
#include <print>
#include <thread>
#include <chrono>
#include <random>
#include <vector>
#include <span>


#include "Ingestion.h"
#include "wal/Wal.h"
#include "dsa/MemTable.h"
#include "dsa/MovingAverage.h"
#include "wal/WriteStaleWal.h"
#include "storage/SSTable/SSTable.h"
#include "storage/SSTable/Sit.h"
#include "storage/Layout/Storage.h"
#include "storage/Compression/Compress.h"

int main() {

   std::println("GooseTSDB started");

   // constexpr int N_TICKS = 50000000;
   // constexpr int N_SYMBOLS = 5000;

   // std::mt19937_64 gen{42};

   // //creates random symbols
   // std::vector<double> weights(N_SYMBOLS);
   // for(int k {0};k < N_SYMBOLS;k++) {
   //    weights[k] = 1.0 / (k + 1);
   // }
   // std::discrete_distribution<uint32_t> symbol(weights.begin(), weights.end());
   // std::uniform_int_distribution<uint64_t> jitter(0, 999);

   // auto dur = std::chrono::duration<double>(0);

   // for(int i {0};i < 5;i++) {
   //    std::vector<Tick> ticks;
   //    ticks.reserve(N_TICKS);
   //    for(int j {0};j < N_TICKS;j++) {
   //       ticks.push_back({(uint64_t)(1000000 + j) + jitter(gen), 10000 + jitter(gen), (uint32_t)jitter(gen), symbol(gen)});
   //    }

   // //will contain global indexes, will become std vector<SSTable>
   // Storage storage;

   //    auto start = std::chrono::steady_clock::now();

   //    {
   //       MemTable mem_table{storage};

   //       for(auto& tick : ticks) {
   //          mem_table.append(tick);
   //       }
   //    }

   //    auto elapsed = std::chrono::steady_clock::now() - start;
   //    std::println("Time taken {}", elapsed);
   //    
   //    dur += std::chrono::duration<double>(elapsed);
   // }


 
   //std::println("{:.2f}M ticks/s", ((N_TICKS * 5) / dur.count()) / 1e6);
   // auto point = Sit::lookup(storage, 1, 1000000, 1100000);
   // std::println("Point lookup rows: {}", point.size());
   // for(const auto& r : point) {
   //    std::println("ts={} price={} vol={} sym={}", r.ts, r.price, r.vol, r.symbol_id);
   // }


   //delta of delta testing

   std::array<uint64_t, 7> ts = {1496163646, 1496163676, 1496163706, 1496163735, 1496163765, 1496163735, 1496163785};

   std::vector<int64_t> res(7);
   size_t used {0};
   compression::encode_ts_price(std::span<uint64_t>(ts).first(7), res, used);


   //compression round trip testing
   //ticks go through the pipeline to disk, then every block is decoded and compared

   {
      std::mt19937_64 gen{42};
      std::uniform_int_distribution<uint64_t> jitter(0, 999);
      std::uniform_int_distribution<uint32_t> symbol(0, 49);

      Storage storage;

      {
         MemTable mem_table{storage};

         for(int j {0};j < 100000;j++) {
            Tick t {(uint64_t)(1000000 + j) + jitter(gen), 10000 + jitter(gen), (uint32_t)jitter(gen), symbol(gen)};
            mem_table.append(t);
         }
      }

      size_t blocks {0}, mismatched {0};

      //to verify if original blocks match encoded -> decoded blocks
      for(auto& sstable : storage.sstables) {
         for(auto& [symbol_id, meta] : sstable->file_metadata) {
            for(const BlockMeta& bmeta : meta.block_data) {

               Block decoded = compression::decode(storage, sstable.get(), symbol_id, bmeta.block_id);
               const Block& original = sstable->blocks[bmeta.block_id];
               blocks++;


               bool same = decoded.elements == original.elements;
               for(size_t i {0};same && i < original.elements;i++) {
                  same = decoded.ts[i] == original.ts[i]
                     && decoded.price[i] == original.price[i]
                     && decoded.vol[i] == original.vol[i];
               }

               if(!same) mismatched++;
            }
         }
      }

      std::println("{} blocks, {} failed encode/decodes", blocks, mismatched);


      //This used to validate the index encoding/decoding on disk
      //The test writes index to disk and later reads and loads back into memory
      //and compares original to read
      size_t indexes {0}, index_mismatched {0};

      for(auto& sstable : storage.sstables) {

         if(sstable->file_metadata.empty()) continue;

         //every symbol in an sstable shares the same file number
         const uint64_t file_number = sstable->file_metadata.begin()->second.file_number;
         auto decoded = compression::decode_index(storage, file_number);
         indexes++;

         bool same = decoded.size() == sstable->file_metadata.size();


         for(const auto& [symbol_id, original] : sstable->file_metadata) {
            if(!same) break;

            auto it = decoded.find(symbol_id);
            if(it == decoded.end()) {
               same = false;
               break;
            }

            const FileMetadata& meta = it->second;

            same = meta.file_number == original.file_number
               && meta.max_ts == original.max_ts
               && meta.min_ts == original.min_ts
               && meta.max_price == original.max_price
               && meta.min_price == original.min_price
               && meta.block_offset == original.block_offset
               && meta.block_data.size() == original.block_data.size();

            for(size_t i {0};same && i < original.block_data.size();i++) {
               const BlockMeta& a = meta.block_data[i];
               const BlockMeta& b = original.block_data[i];

               same = a.block_id == b.block_id
                  && a.bmax_ts == b.bmax_ts
                  && a.bmin_ts == b.bmin_ts
                  && a.bmax_price == b.bmax_price
                  && a.bmin_price == b.bmin_price
                  && a.elements == b.elements
                  && a.ts_offset == b.ts_offset
                  && a.ts_size == b.ts_size
                  && a.price_offset == b.price_offset
                  && a.price_size == b.price_size
                  && a.vol_offset == b.vol_offset
                  && a.vol_size == b.vol_size;
            }
         }

         if(!same) index_mismatched++;
      }

      std::println("{} indexes, {} failed index encode/decodes", indexes, index_mismatched);
   }








   return 0;

}
