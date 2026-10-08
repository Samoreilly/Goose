
#include <print>
#include <filesystem>
#include <iostream>
#include <string>
#include <cstdio>
#include <charconv>
#include <string_view>

#include "Wal.h"


/*
This classes purposes is to move old .wal files to immutable storage in the background,
it's simply a file with the name that is less than file_counter
This means its old and not being used
*/

class WriteStateWal {

    Wal& wal;

public:

    WriteStateWal(Wal& w) : wal(w) {}

    //this is the default place for .wal files, may make a config later
    //so it's customisable
    const std::filesystem::path file_path{wal.folder_name}; 


    void move_wal() {
        
        if(!std::filesystem::exists(file_path)) {
            std::println(stderr, "{}", file_path.filename().string());
            std::filesystem::create_directories(file_path);
            std::println("<WriteStaleWal.h> Created directory: {}", file_path.c_str());
        }

        for(auto const& entry : std::filesystem::directory_iterator{file_path}) {
            auto file_name_wext = entry.path().filename().native();
            
            //gives the strings name excluding the extension
            std::string_view file_name = 
                std::string_view(file_name_wext).substr(0, file_name_wext.length() - 4);

            size_t file_number {0}; 
            std::ignore = std::from_chars(file_name.data(), file_name.data() + file_name.size(), file_number);

            //If true, it is not in use
            if(file_number < wal.file_counter) {
                //move to SSTABLE
            }

        }

    }


};
