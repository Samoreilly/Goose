#pragma once

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

class Compaction {

   Storage& storage;

   std::thread comp;

   void search();
   void compact(const std::vector<std::pair<std::string, std::string>> candidates);
   bool evaluate(const auto& cand_one, const auto& cand_two) const;

public:

   Compaction(Storage& s) : storage(s) {}

   void start();



};
