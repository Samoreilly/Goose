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
    
    
    /*
        Writing this was slightly difficult so //NOTE: to help me understand when I come back to this
        It's practically a queue, the right bits being the front.
        Unnecessary bits are removed from Value, then appended onto leftside of stream
        by shifting to align with the bits then ORing to add.
        
        storage & 0xFF grabs the right most byte to append
    */
    struct BitWriter {
        std::vector<uint8_t> out;

        uint64_t store {0};//holds bits
        uint32_t n {0};//holds number of bits

        //this cuts off unneccesary leading bits that dont affect the value and appends
        //to the left side of the stream
        //this is to pack bits together, a control prefix will be used to
        //know much bits to read
        void write(uint64_t value, uint32_t bits) {

            //split wide writes, 1ULL << 64 is UB and store can only hold 64 bits
            //low half first to keep the same lsb first order
            if(bits > 32) {
                write(value & 0xFFFFFFFF, 32);
                write(value >> 32, bits - 32);//writes remaining bits
                return;
            }

            //e.g. value = 0b00101101, bits = 3
            // 1ULL << bits = 0b0000 1000
            // prev - 1 = 0b0000 0111
            
            value &= (1ULL << bits) - 1;
            
          
            //shift incoming value over used for bits to be be adjacent
            store |= (value << n);
            n += bits;

            while(n >= 8) {
                //get the last byte
                out.push_back(static_cast<uint8_t>(store & 0xFF));
                store >>= 8;//shifts to right
                n -= 8; 
            }
            
        }
        void flush() {
            if(n == 0) return;
            uint8_t byte = store & 0xFF;
            store = 0, n = 0; 
            out.push_back(byte);
        }

    };


    void encode_vol(std::span<const uint32_t> data, std::vector<int32_t>& arr, size_t& used);
    void encode_ts_price(std::span<const uint64_t> data, std::vector<int64_t>& arr, size_t& used);

    void write_ts_price_vol(BitWriter& writer, const std::span<const int64_t> dods);
    void write_vol(BitWriter& writer, const std::span<const int32_t> dods);


}

