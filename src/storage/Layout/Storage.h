
#include "Block.h"
#include "../FileMetadata.h"
#include "../../helpers/Convert.h"

/*
There will be 1024 rows per block. 20480 bytes per block
8192 bytes for timestamp and price, 4096 bytes for vol

An index will be in the footer of the file, this will contain Block metadata
such as the max/min timestamp/price, this will allow the reader(for queries) to skip
blocks
*/

class Storage {


public:

    Storage() {
        //the number of symbol_ids found since ingestion
        metadata.resize(ticker_to_id.size());
    }

    std::vector<FileMetadata> metadata;

};
