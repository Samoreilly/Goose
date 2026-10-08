
#include <inttypes.h>
#include <cstddef>
#include <vector>

/*
Prior to the memtable flush, it will be written to an immutable SSTABLE on disk and file metadata in a hierarchical vector
*/
struct SSTableFileMetadata {
    uint64_t file_number {0};
    size_t size;

    //purpose is to check ranges quickly to avoid looping through sstable
    uint64_t max_key {0}, min_key {0};
    uint64_t max_ts {0}, min_ts {0};    
};


class FileMetadata {

public:

    //Contains sstable metadata
    std::vector<SSTableFileMetadata> sst_metadata{100};
};
