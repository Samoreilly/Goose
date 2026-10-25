#pragma once

#include <unordered_map>
#include <string>

enum class KEYWORD {

    TICKER,

    GET,

    L,
    G,
    LT,
    GT,
    EQ,

    MAX,
    MIN,

    AVG,
    SUM,
    COUNT,

    BETWEEN,

    OHLC,
    VWAP

};


static inline const std::unordered_map<std::string_view, KEYWORD> KEYWORDS = {
    
    {"GET", KEYWORD::GET},
    
    //operators are from left side perspective
    {"<", KEYWORD::L},
    {">", KEYWORD::G},
    
    {"<=", KEYWORD::LT},
    {">=", KEYWORD::GT},
    {"=", KEYWORD::EQ},

    {"MAX", KEYWORD::MAX},
    {"MIN", KEYWORD::MIN},

    {"AVG", KEYWORD::AVG},
    {"SUM", KEYWORD::SUM},
    {"COUNT", KEYWORD::COUNT},

    {"BETWEEN", KEYWORD::BETWEEN},

    {"OHLC", KEYWORD::OHLC},
    {"VWAP", KEYWORD::VWAP},

};


