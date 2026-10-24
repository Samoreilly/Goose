#pragma once

#include <string_view>
#include <inttypes.h>
#include <zlib.h>

class MemTable;

/*
This class is reponsible for replaying WAL files
and runs at program start before anything else
*/

class Replay {

    std::string_view folder_name = "gdb/wal/";

public:

    Replay() {}
   
    uint32_t crc = crc32(0L, Z_NULL, 0);
     
    /*
    Plays .wal Ticks through pipeline
    */
    void stream(MemTable& mem_table);
    bool read(const auto& file_entry, auto& stream);

};
