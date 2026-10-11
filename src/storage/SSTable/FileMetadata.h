
#include <inttypes.h>
#include <cstddef>
#include <vector>

/*
Prior to the memtable flush, it will be written to an immutable SSTABLE on disk and file metadata in a hierarchical vector
*/
struct FileMetadata {
    uint64_t file_number {0};
    size_t size;

    //purpose is to check ranges quickly to avoid scanning blocks
    uint64_t max_key {0}, min_key {0};
    uint64_t max_ts {0}, min_ts {0};    
};


