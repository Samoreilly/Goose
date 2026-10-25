
#include <print>
#include <cctype>
#include <chrono>
#include <charconv>

#include "Lexer.h"
#include "../Keywords.h"

Query Lexer::parse(const std::string& query) {

    Query stream{};

    const size_t size = query.size();
    size_t i {0};

    auto str_view = [&query](const size_t start, const size_t end) {
        return std::string_view(query).substr(start, end - start);
    };

    while(i < size) {

        //commas separate columns, tickers and ranges
        while(i < size && (std::isspace(query[i]) || query[i] == ',')) i++;
        if(i == size) break;

        //* returns every column
        if(query[i] == '*') {
            stream.columns = {"ts", "price", "vol"};
            i++;
            continue;
        }

        size_t start {i};
        while(i < size && std::isalnum(query[i])) i++;

        //dates are the only words followed by a '-' e.g. 30-01-2005..10-06-2026
        if(i < size && query[i] == '-') {
            auto range = handle_timestamp(query, start, i, size);
            if(!range) return {};

            stream.ranges.push_back(*range);
            continue;
        }

        //not a word, so either an operator e.g. <= or an unexpected character
        if(i == start) {
            while(i < size && (query[i] == '<' || query[i] == '>' || query[i] == '=')) i++;

            if(i == start || !KEYWORDS.contains(str_view(start, i))) {
                std::println("Unexpected character '{}' at {}", query[start], start);
                return {};
            }
        }

        auto word = str_view(start, i);
        auto it = KEYWORDS.find(word);

        if(it != KEYWORDS.end()) {
            //GET and BETWEEN only mark where the columns/tickers and the ranges start
            if(it->second != KEYWORD::GET && it->second != KEYWORD::BETWEEN) {
                std::println("{} is not supported yet", word);
                return {};
            }
        }else if(word == "ts" || word == "price" || word == "vol") {
            stream.columns.push_back(std::string(word));
        }else if(std::isalpha(word[0])) {
            stream.symbols.push_back(std::string(word));
        }else {
            std::println("Expected a ticker at {}, got {}", start, word);
            return {};
        }

    }


    std::print("Columns: ");
    for(const auto& c : stream.columns) {
        std::println("{}", c);
    }

    std::print("Symbols");
    for(const auto& s : stream.symbols) {
        std::println("{}", s);

    }

    std::print("Ranges");
    for(const auto& t : stream.ranges) {
        std::println("Start: {} End: {}", t.start, t.end);
    }

    return stream;
}

//parses dd-mm-yyyy..dd-mm-yyyy starting at start, idx_ref is moved past the range
//end is the last second of its day so the range is inclusive
std::optional<Time> Lexer::handle_timestamp(const std::string& query, size_t start, size_t& idx_ref, size_t len) {

    using namespace std::chrono;

    //parses a single dd-mm-yyyy date into seconds since epoch
    auto parse_date = [&query, len](size_t& idx) -> std::optional<size_t> {

        uint64_t dmy[3] {0, 0, 0};

        for(size_t part {0};part < 3;part++) {
            //day and month are followed by '-'
            if(part > 0) {
                if(idx >= len || query[idx] != '-') return std::nullopt;
                idx++;
            }

            auto [ptr, ec] = std::from_chars(query.data() + idx, query.data() + len, dmy[part]);
            if(ec != std::errc{}) return std::nullopt;

            idx = ptr - query.data();
        }

        const year_month_day date{year(static_cast<int>(dmy[2])), month(dmy[1]), day(dmy[0])};

        //size_t can't hold dates before epoch
        if(!date.ok() || date.year() < year(1970)) return std::nullopt;

        return static_cast<size_t>(sys_seconds(sys_days(date)).time_since_epoch().count());
    };

    size_t idx {start};
    auto ts_start = parse_date(idx);

    const bool has_dots = idx + 1 < len && query[idx] == '.' && query[idx + 1] == '.';
    if(ts_start && has_dots) idx += 2;

    auto ts_end = (ts_start && has_dots) ? parse_date(idx) : std::nullopt;

    //range has to end at a space, comma or the end of the query
    if(!ts_end || (idx < len && !std::isspace(query[idx]) && query[idx] != ',')) {
        std::println("Invalid date range at {}, expected dd-mm-yyyy..dd-mm-yyyy", start);
        return std::nullopt;
    }

    *ts_end += 24 * 60 * 60 - 1;

    if(*ts_start > *ts_end) {
        std::println("Date range at {} starts after it ends", start);
        return std::nullopt;
    }

    idx_ref = idx;
    return Time{*ts_start, *ts_end};
}
