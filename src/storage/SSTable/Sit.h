
#include <inttypes.h>
#include "../../dsa/MemTable.h"

/*
Columnar
Sorted Integer Table as opposed to sorted integer table
Keys will be the id that is converted from the ticker at ingestion stage
*/


//Templated size to take in variable "used" in chunks
template<size_t SIZE>
struct SSTable {
    uint64_t ts[SIZE];
    uint64_t price[SIZE];
    uint32_t vol[SIZE];
};

class Sit {

    MemTable memtable;
    
public:

    Sit(MemTable&& m) : memtable(m) {}

};
