#pragma once

#include <inttypes.h>
#include <string>

struct Data {
    uint64_t ts;
    uint64_t price;
    std::string symbol;
    uint32_t size;

};

struct Tick {
    uint64_t ts;        
    uint64_t price;     
    uint64_t symbol_id; 
    uint32_t size;  
};
