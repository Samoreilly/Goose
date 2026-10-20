
#include <filesystem>
#include <print>
#include <unordered_set>

#include "Compaction.h"

namespace fs = std::filesystem;

void Compaction::start() { 
   std::println("Starting Compaction");
   
   comp = std::thread(&Compaction::search, std::ref(*this));
}

void Compaction::search() {

   std::vector<std::pair<std::string, std::string>> candidates;
   candidates.reserve(storage.file_counter);

   std::unordered_set<std::string> used;

   const fs::path file_path{storage.folder_name}; 

   if(!fs::exists(file_path)) {
      std::println("Created gdb");
      fs::create_directories(file_path);
   }

   for(const auto& dir : fs::directory_iterator(file_path)) {
      const auto& path = dir.path();
   
      auto cand_one = compression::get_index(path);

      for(const auto& pot_cand : fs::directory_iterator(file_path)) {
         const std::string one_path = dir.path().stem().native();
         const std::string two_path = pot_cand.path().stem().native();

         if((pot_cand.file_size() + dir.file_size()) >= MAX_COMPACTION_FILE_SIZE) {
            continue;
         }

         //not same files
         if(one_path != two_path) {

            auto cand_two = compression::get_index(pot_cand);
            bool merge = evaluate(cand_one, cand_two);

            if(!merge || used.contains(one_path) || used.contains(two_path)) continue;

            used.insert({one_path, two_path});
            candidates.emplace_back(one_path, two_path);
         }

      }
   }

   compact(std::move(candidates));
}

void Compaction::compact(const std::vector<std::pair<std::string, std::string>> candidates) {

   //sstable_sec will be merged into sstable_one
   for(const auto& cand : candidates) {
      auto sstable_one = compression::load_sstable(cand.first); 
      auto sstable_sec = compression::load_sstable(cand.second);

      //find index for block


   }

}


bool Compaction::evaluate(const auto& cand_one, const auto& cand_two) const {
   
   int merge {0}, no_merge {0};

   for(auto it_one = cand_one.begin(); it_one != cand_one.end();it_one++) {
      
      auto it_two = cand_two.find(it_one->first);
      //check if both maps have same symbol
      if(it_two == cand_two.end()) continue;

      //FileMetadata for same symbol
      const auto& a = it_one->second;
      const auto& b = it_two->second;

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

   }

   //a heuristic to signify a good file to merge
   return merge >= no_merge * 2;
}







