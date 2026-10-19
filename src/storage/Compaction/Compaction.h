
#include <thread>

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

class Compaction {

   Storage& storage;

   std::thread comp;
   void compact();
   
   void evaluate(const auto& cand_one, const auto& cand_two) const;

public:

   Compaction(Storage& s) : storage(s) {}


   void start();



};
