#pragma once

#include <memory>
#include <shared_mutex>
#include <filesystem>
#include <string>
#include <string_view>
#include <charconv>
#include <print>

#include "../SSTable/SSTable.h"

class Storage {

public:

    //.back() is most recent SSTable
    //search backwards to find data
    std::vector<std::unique_ptr<SSTable>> sstables;
    std::shared_mutex mu;

    //sstable file counter
    //only the flusher creates files so this doesnt need to be atomic
    size_t file_counter {0};
    std::string folder_name = "gdb/goose/";

    Storage() {
        //arbitrary, will change later
        sstables.reserve(1000);
        load_file_counter();
    }

    //on start, file_counter is set to one past the highest file number
    //so old files dont get overwritten after a restart
    void load_file_counter() {

        const std::filesystem::path file_path{folder_name};

        if(!std::filesystem::exists(file_path)) {
            std::filesystem::create_directories(file_path);
            std::println("<Storage.h> Created directory: {}", file_path.c_str());
            return;
        }

        for(const auto& entry : std::filesystem::directory_iterator{file_path}) {
            
            //gives the files name excluding the extension
            auto file_name_wext = entry.path().stem().native();
            std::string_view file_name {file_name_wext};

            size_t file_number {0};
            //int to string
            auto [ptr, ec] = std::from_chars(file_name.data(), file_name.data() + file_name.size(), file_number);
            
            //not one of our files
            if(ec != std::errc{}) continue;

            if(file_number >= file_counter) {
                file_counter = file_number + 1;
            }

        }

    }


};

