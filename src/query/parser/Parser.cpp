#include "Parser.h"
#include "../../engine/helpers/Convert.h"

void Parser::parse(Query query) {

   QueryResult result;

   const auto& symbols = query.symbols;
   const auto& range  = query.range;

   //scan through queries symbols in all sstables
   //checking global and min/max for overlap
   for(const auto& ss : storage.sstables) {
      const auto& meta = ss->file_metadata;

      for(const auto& symbol_as_string : symbols) {
         
         //convert ticker (string) to id (int)
         uint32_t sym_as_int = ticker_to_id[symbol_as_string];
         auto& symbol_data = meta.find(sym_as_int)->second;

         //check for global overlap
         if(!(range.start <= symbol_data.max_ts && range.end >= symbol_data.min_ts)) continue;
            
         for(const auto& block : symbol_data.block_data) {
            //check for block local overlap
            if(!(range.start <= block.bmax_ts && range.end >= block.bmin_ts)) continue;
            
            //gives us the block for data that overlaps using block_id
            const auto& blocks = ss->blocks[block.block_id];

            for(size_t i {0};i < blocks.elements;i++) {
               if(blocks.ts[i] < range.start || blocks.ts[i] > range.end) break;
               
               result.ts.push_back(blocks.ts[i]);
               result.price.push_back(blocks.price[i]);
               result.vol.push_back(blocks.vol[i]);
            }
         }
      }
   }

}
