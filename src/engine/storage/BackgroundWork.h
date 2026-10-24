#pragma once

#include "../dsa/MemTable.h"


struct SortMetadata {
    uint32_t data;//amount of ticks per symbol. Used in Counts vector in BackgroundWork.cpp
    bool sorted;
};

/*
This will handle memtable clean up for now
*/
class MemTable;
void cleanup_memtable(MemTable& mem);



