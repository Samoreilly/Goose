#pragma once

#include <cstddef>
#include <cstdint>
#include <string.h>
#include <zlib.h>
#include "../DataTypes.h"
#include <filesystem>


//NOTE: 8mb arbitrary value, will tune it later
#define MAX_FILE_SIZE_B 4000000 
#define BUFFER_SIZE_B 1000000 //1mb for staging buffer

/*
Write ahead log for backup
The wal will consist of file names incrementing e.g. 000001.wal 
If it meets some size, it will be flushed and then can be deleted
Stored in gdb/wal

WAL logs will be periodically flushed to SS Table and removed from logs
*/

class Wal {

    size_t file_counter {0};
    std::string folder_name = "gdb/wal/";
    
    int fd {-1};
    size_t current_size {0};

    uint8_t buffer[BUFFER_SIZE_B];
    size_t used {0};

    //To check for data corruption during file storage or transfering
    uint32_t crc = crc32(0L, Z_NULL, 0);
    
    bool write_bytes(int fd, uint8_t buffer[], size_t count);
    bool fsync_dir(std::filesystem::path file_path);

public:

    Wal() {}

    void append(const Tick& data);
    bool flush();

};


