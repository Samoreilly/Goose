#pragma once

#include <inttypes.h>
#include <cstddef>
#include <limits>
#include <vector>

/*
Prior to the memtable flush, it will be written to an immutable SSTABLE on disk and file metadata in a hierarchical vector
*/
struct BlockMeta {
    
    uint64_t block_id;
    //These are local min/maxes for this block
    uint64_t bmax_price {0}, bmin_price {std::numeric_limits<uint64_t>::max()};
    uint64_t bmax_ts {0}, bmin_ts {std::numeric_limits<uint64_t>::max()};    
        
    uint32_t file_offset;//where block starts
    uint32_t compressed_size;//size of compressed block
    uint32_t elements;//number of elements in block(necessary for half filled blocks)

    size_t ts_offset {0}, ts_size {0};
    size_t price_offset {0}, price_size {0};
    size_t vol_offset {0}, vol_size {0};

};



/*
Im using two scope min/maxes to optimise searches. The symbol scope contains min/max for all blocks for that symbol_id,
whereas the local scope is the min/max for that specific block
*/
struct FileMetadata {
    uint64_t file_number {0};
        
    std::vector<BlockMeta> block_data;

    //Symbol scope min/max
    uint64_t max_price {0}, min_price {std::numeric_limits<uint64_t>::max()};
    uint64_t max_ts {0}, min_ts {std::numeric_limits<uint64_t>::max()};

    size_t block_offset {0};
};


