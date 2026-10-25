#pragma once

#include <string>
#include <vector>
#include <optional>
#include <inttypes.h>

//seconds since epoch
struct Time {
    size_t start {0};
    size_t end {0};
};

struct Query {
    std::vector<std::string> columns;//ts, price, vol
    std::vector<std::string> symbols;//MSFT
    std::vector<Time> ranges;//start..end
};


class Lexer {

    std::optional<Time> handle_timestamp(const std::string& query, size_t start, size_t& idx_ref, size_t len);

public:

    Lexer() {}

    //returns an empty Query if the query is invalid
    Query lex(const std::string& query);

};
