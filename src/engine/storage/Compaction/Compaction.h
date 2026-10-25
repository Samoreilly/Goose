#pragma once

#include <atomic>
#include <thread>
#include <unordered_set>

#include "../Compression/Compress.h"
#include "../Layout/Storage.h"

/*
Compaction is responsbile for merging SSTables(.goose files in this case)
Upon compaction, blocks with the same symbol_id will be merged or added to
adjacently allowing the original file to be removed

Compaction candidates will be decided 3 things.
By creation, the lower the file name, the older.
A size threshold
If there min/max values overlap
*/

#define MAX_COMPACTION_FILE_SIZE 64'000'000
#define MAX_COMPACTIONS_PER_COMPACTIONS 5

class Compaction {

   Storage& storage;

   std::atomic<bool> stop {false};

   std::thread comp;
   void search();

   /*
   compact() notes

   Symbol_id's that dont exist in both files will be appended to the end of blocks,
   those that do exist in both will be merged.
   SSTable index will be updated throughout the compaction
   Both files are merged into a new .goose file, then both originals are removed
   */
   void compact(const std::vector<std::pair<std::string, std::string>> candidates);

   bool evaluate(const auto& cand_one, const auto& cand_two) const;

public:

   Compaction(Storage& s) : storage(s) {}

   ~Compaction() {
      stop = true;
      storage.flag.store(1, std::memory_order_release);
      storage.flag.notify_one();
      comp.join();
   }

   void start();



};
