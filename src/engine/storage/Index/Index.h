#pragma once

#include <inttypes.h>
#include <unordered_map>

/*
This will be a global summary of a file
*/

struct FileManifest {
    uint32_t fmax_ts {0}, fmin_ts {0};
    uint32_t fmax_price {0}, fmin_price {0};
};

