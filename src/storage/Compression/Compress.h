#pragma once

#include <span>

#include "../SSTable/SSTable.h"

/*
Delta of Delta for timestamp and price, currently undecided on the method for vol
*/
namespace compression {

    inline size_t file_counter {0};
    inline std::string folder_name = "gdb/goose";
    

    //pointer to element in Storages SSTables vector
    void compress(SSTable* sstable);
    
    //Delta of Delta
    struct BitWriter {
        std::vector<uint8_t> out;

        int store {0};//holds bits
        int n {0};//holds number of bits

        //this cuts off unneccesary leading bits that dont effect the value
        //this is to pack bits together, a control prefix will be used to
        //know much bits to read
        void write(uint64_t value, uint32_t bits) {
            
            //e.g. value = 0b00101101, bits = 3
            // 1ULL << bits = 0b0000 1000
            // prev - 1 = 0b0000 0111
            
            value &= (1ULL << bits) - 1;
            n += bits;

            if(n >= 8) {
                out.push_back(value);
                n = 0; 
            }
        }

    };


    void encode_ts_price(std::span<const uint64_t> ts, std::vector<int64_t>& arr, size_t& used);
    void write_ts_price(BitWriter& writer, const std::span<const int64_t> dods);

}

