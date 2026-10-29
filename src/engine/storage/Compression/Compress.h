#pragma once

#include <span>
#include <memory>
#include <filesystem>

#include "../SSTable/SSTable.h"
#include "../Layout/Storage.h"
#include "../Index/Index.h"

/*
Delta of Delta for timestamp and price, Delta for vol
*/

//File format
// BLOCK 1 | BLOCK 2 | SUMMARY | INDEX | 8 byte offset to index | 8 byte offset to summary
namespace compression {


    //pointer to element in Storages SSTables vector
    bool compress(Storage& storage, SSTable* sstable);
    
    
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

    //opposite of BitWriter, bytes are added to the left side of store
    //and bits are taken from the right
    struct BitReader {
        std::span<const uint8_t> in;
        size_t pos {0};//next byte to read

        uint64_t store {0};//holds bits
        uint32_t n {0};//holds number of bits

        uint64_t read(uint32_t bits) {

            //same split as write, low half comes first
            if(bits > 32) {
                uint64_t low = read(32);
                uint64_t high = read(bits - 32);
                return low | (high << 32);
            }

            while(n < bits) {
                //past the end reads as 0s, only padding should hit this
                uint64_t byte = pos < in.size() ? in[pos++] : 0;
                store |= (byte << n);
                n += 8;
            }

            uint64_t value = store & ((1ULL << bits) - 1);
            store >>= bits;
            n -= bits;

            return value;
        }
    };

    void encode_vol(std::span<const uint32_t> data, std::vector<int32_t>& arr, size_t& used);
    void encode_ts_price(std::span<const uint64_t> data, std::vector<int64_t>& arr, size_t& used);

    void write_ts_price_vol(BitWriter& writer, const std::span<const int64_t> dods);
    void write_vol(BitWriter& writer, const std::span<const int32_t> dods);

    void read_ts_price_vol(BitReader& reader, std::span<int64_t> dods);
    void read_vol(BitReader& reader, std::span<int32_t> dods);

    void decode_ts_price(std::span<const int64_t> arr, std::span<uint64_t> data);
    void decode_vol(std::span<const int32_t> arr, std::span<uint32_t> data);

    //block_id is the block we want to decode
    Block decode(Storage& storage, SSTable* sstable, uint64_t symbol_id, uint64_t block_id);

    //will contain all metadata for blocks 
    void encode_index(std::vector<uint64_t>& out, const std::unordered_map<uint64_t, FileMetadata>& metadata, size_t base_offset);

    //fills the file wide summary (min/max ts and price) and appends it to out
    FileManifest encode_summary(std::vector<uint64_t>& out, const std::unordered_map<uint64_t, FileMetadata>& metadata);
    
    //decoding SSTable's will occur in the background on start.
    //Undecided how these will be loaded on start
    std::unordered_map<uint64_t, FileMetadata> decode_index(Storage& storage, const uint64_t file_counter);



    /*
        Compaction related functions 
    */

    //returns index from file named "file_path"
    std::unordered_map<uint64_t, FileMetadata> get_index(std::filesystem::path file_path);

    //decodes file named file_path back into an sstable, index and all blocks
    std::unique_ptr<SSTable> load_sstable(std::filesystem::path file_path);


}

