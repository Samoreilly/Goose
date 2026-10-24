#pragma once

#include <chrono>
#include <cstddef>
#include <cstdint>
#include <locale>
#include <stop_token>
#include <string.h>
#include <zlib.h>
#include <filesystem>
#include <thread>
#include <mutex>
#include <print>
#include <atomic>

#include "../DataTypes.h"

//NOTE: 8mb arbitrary value, will tune it later

#define BUFFER_SIZE_B 1000000 //1mb for staging buffer

/*
Write ahead log for backup
The wal will consist of file names incrementing e.g. 000001.wal 
If it meets some size, it will be flushed and then can be deleted
Stored in gdb/wal

WAL logs will be periodically flushed to SS Table and removed from logs
*/

class Wal {

    int fd {-1};
    size_t current_size {0};

    uint8_t buffer[BUFFER_SIZE_B];
    size_t used {0};

    //To check for data corruption during file storage or transfering
    uint32_t crc = crc32(0L, Z_NULL, 0);
    
    bool write_bytes(int fd, uint8_t buffer[], size_t count);
    bool fsync_dir(std::filesystem::path file_path);

    void load_file_counter();

public:

    Wal() {
        if(!std::filesystem::is_directory(folder_name)) {
            std::filesystem::create_directories(folder_name);
            std::println("Created directory: gdb/wal/");
        }

        load_file_counter();
    }

    ~Wal() {
        //flush remaining data in buffer
        flush();
    }

    size_t file_counter {0};
    std::string folder_name = "gdb/wal/";

    //true while old WAL files are being replayed on startup
    //stops remove_stale() deleting them before every replayed tick is in an SSTable
    std::atomic<bool> replaying {false};
 
    void append(const Tick& data);
    bool flush();
    bool rotate();

    void remove_stale();


};









