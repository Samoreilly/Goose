
#include <filesystem>
#include <print>

#include "Compaction.h"

namespace fs = std::filesystem;

void Compaction::start() { 
   std::println("Starting Compaction");
   
   comp = std::thread(&Compaction::compact, std::ref(*this));
}

void Compaction::compact() {

   const fs::path file_path {storage.folder_name}; 

   if(!fs::exists(file_path)) {
      std::println("Created gdb");
      fs::create_directories(file_path);
   }

   for(const auto& dir : fs::directory_iterator(file_path)) {
      const auto& path = dir.path();
   
      auto cand_one = compression::get_index(path);

      for(const auto& pot_cand : fs::directory_iterator(file_path)) {

         if(!fs::equivalent(dir.path(), pot_cand.path())) {
            auto cand_two = compression::get_index(pot_cand);

            evaluate(cand_one, cand_two);

         }
      }

   }

}


void Compaction::evaluate(const auto& cand_one, const auto& cand_two) const {

}







