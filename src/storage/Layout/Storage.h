#pragma once

#include <memory>
#include <shared_mutex>

#include "../SSTable/SSTable.h"

class Storage {

public:

    //.back() is most recent SSTable
    //search backwards to find data
    std::vector<std::unique_ptr<SSTable>> sstables;
    std::shared_mutex mu;

    Storage() {
        //arbitrary, will change later
        sstables.reserve(1000);
    }

    

};

