
#include <atomic>
#include <filesystem>
#include <print>
#include <unordered_set>
#include <ranges>
#include <algorithm>
#include <tuple>
#include <semaphore>

#include "Compaction.h"
#include "../SSTable/Sit.h"

namespace fs = std::filesystem;
using namespace std::chrono_literals;

void Compaction::start() { 
   std::println("Starting Compaction");
   comp = std::thread(&Compaction::search, std::ref(*this));
}

void Compaction::search() {

   while(true) {
 
      storage.flag.wait(0, std::memory_order_acquire);
      storage.flag = 0;  
      
      if(stop) return;

      std::vector<std::pair<std::string, std::string>> candidates;
      candidates.reserve(storage.file_counter);

      std::unordered_set<std::string> used;

      const fs::path file_path{storage.goose_folder_name}; 

      if(!fs::exists(file_path)) {
         std::println("Created gdb");
         fs::create_directories(file_path);
      }

      for(const auto& dir : fs::directory_iterator(file_path)) {
         const auto& path = dir.path();
      
         if(path.extension() != ".goose") continue;

         //5 compaction max 
         if(candidates.size() == MAX_COMPACTIONS_PER_COMPACTIONS) break;

         auto cand_one = compression::get_index(path);

         for(const auto& pot_cand : fs::directory_iterator(file_path)) {
            const std::string one_path = dir.path().stem().native();
            const std::string two_path = pot_cand.path().stem().native();

            if(pot_cand.path().extension() != ".goose") continue;
            if((pot_cand.file_size() + dir.file_size()) >= MAX_COMPACTION_FILE_SIZE) {
               continue;
            }

            //not same files
            if(one_path != two_path) {

               auto cand_two = compression::get_index(pot_cand);
               bool merge = evaluate(cand_one, cand_two);

               if(!merge || used.contains(one_path) || used.contains(two_path)) continue;

               used.insert({one_path, two_path});
               candidates.emplace_back(dir.path().native(), pot_cand.path().native());
            }

         }
      }

      compact(std::move(candidates));
   
      std::println("Compaction finished");

   }
}



void Compaction::compact(const std::vector<std::pair<std::string, std::string>> candidates) {


   //both files are merged into a new sstable
   for(const auto& cand : candidates) {
      auto sstable_one = compression::load_sstable(cand.first); 
      auto sstable_two = compression::load_sstable(cand.second);

      //every row from both files, create_blocks rebuilds the blocks and index
      std::vector<ChunkSort> rows;

      for(const SSTable* sstable : {sstable_one.get(), sstable_two.get()}) { 
         for(const auto& [key, val] : sstable->file_metadata) {
            for(const auto& bmeta : val.block_data) {

               const Block& block = sstable->blocks[bmeta.block_id];
               
               for(size_t i {0};i < block.elements;i++) {
                  rows.push_back({block.ts[i], block.price[i], block.vol[i], static_cast<uint32_t>(key)});
               }
            }
         }
      }

      //create_blocks reads rows[0]
      if(rows.empty()) continue;

      //create_blocks needs each symbol together and in ts order
      std::sort(rows.begin(), rows.end(), [](const ChunkSort& a, const ChunkSort& b) {
         return std::tie(a.symbol_id, a.ts) < std::tie(b.symbol_id, b.ts);
      });

      auto merged = std::make_unique<SSTable>();
      Sit::create_blocks(merged, rows);
      bool success = compression::compress(storage, merged.get());

      if(!success) {
         std::println("Failed to compact {} into {}", cand.second, cand.first);
         continue;
      }
      
      fs::remove(fs::path(cand.second));
      std::println("Compacted {} into {}", cand.second, cand.first);
   
   }

}


bool Compaction::evaluate(const auto& cand_one, const auto& cand_two) const {
   
   int merge {0}, no_merge {0};

   for(const auto& [key, val] : cand_two) {
      auto it = cand_one.find(key);
      if(it == cand_one.end()) {
         no_merge += val.block_data.size();
      }
   }

   for(auto it_one = cand_one.begin(); it_one != cand_one.end();it_one++) {
      
      auto it_two = cand_two.find(it_one->first);
      //check if both maps have same symbol
      if(it_two == cand_two.end()) {
         no_merge += it_one->second.block_data.size();
         continue;
      }

      //FileMetadata for same symbol
      const auto& a = it_one->second;
      const auto& b = it_two->second;

      //scan for mergable blocks
      for(const auto& a_block : a.block_data) {
         bool overlaps {false};
      
         for(const auto& b_block : b.block_data) {
            if(a_block.bmin_ts <= b_block.bmax_ts && b_block.bmin_ts <= a_block.bmax_ts) {
               overlaps = true;
               break;
            }
         }

         if(overlaps) merge++;
         else no_merge++;
      }

      //scan for mergable blocks
      for(const auto& b_block : b.block_data) {
         bool overlaps {false};

         for(const auto& a_block : a.block_data) {
            if(b_block.bmin_ts <= a_block.bmax_ts && a_block.bmin_ts <= b_block.bmax_ts) {
               overlaps = true;
               break;
            }
         }

         if(overlaps) merge++;
         else no_merge++;
      }

   }

   //a heuristic to signify a good file to merge
   return merge >= no_merge * 2;
}







