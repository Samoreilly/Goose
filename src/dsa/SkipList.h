#pragma once

#include "../ingestion.h"

#define SKIP 50

struct L2 {
    uint64_t tickr;
    L2* next;
    L2* prev;
    //buffer
    //segment
};

struct L1 {
    uint64_t tickr;
    L1* next;
    L2* down;
};


class SkipList {



public:

    SkipList() {

    }

    void append(const Tick& tick);
    void find(const uint64_t tick);
};
