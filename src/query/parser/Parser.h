#pragma once

#include "../lexer/Lexer.h"
#include "../../engine/storage/Layout/Storage.h"


struct QueryResult {
    std::string symbol;
    std::vector<uint64_t> ts;
    std::vector<uint64_t> price;
    std::vector<uint64_t> vol;
};

/*
Each symbols gets it own set of results as opposed to being merged
*/
class Parser {

    Storage& storage;

public:

    Parser(Storage& s) : storage(s) {}

    void parse(Query query);

};
