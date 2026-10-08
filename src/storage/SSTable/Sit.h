
#include <inttypes.h>
#include "../../dsa/MemTable.h"

/*
Columnar
Sorted Integer Table as opposed to sorted integer table
Keys will be the id that is converted from the ticker at ingestion stage
*/


//Templated size to take in variable "used" in chunks

struct SSTable {
    std::vector<int64_t> ts;
    std::vector<uint64_t> price;
    std::vector<uint32_t> vol;
};

class Sit {

    SSTable table;   
    //moves sorted chunk data to columnar arrays
    void transfer_to_ss(const std::vector<ChunkSort>& m);

public:

    //must provide exact types
    explicit Sit(const std::vector<ChunkSort>& sorted_chunks, MemTable&& m) {
        m.sort();
        table.ts.resize(m.total_size);
        table.price.resize(m.total_size);
        table.vol.resize(m.total_size);

        transfer_to_ss(sorted_chunks);

    }
    
};
