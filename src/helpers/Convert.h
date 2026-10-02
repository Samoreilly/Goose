
#include <unordered_map>
#include <string>
#include <inttypes.h>


//used to sequentially & sequentially give tickers an id e.g. [APPL -> 1]
static std::unordered_map<std::string, uint32_t> ticker_to_id{};
static std::unordered_map<uint32_t, std::string> id_to_ticker{};

static uint64_t ticker_count {0};

