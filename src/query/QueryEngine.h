#pragma once

#include <thread>
#include <print>
#include <array>
#include <vector>
#include <mutex>
#include <atomic>
#include <semaphore>
#include <string>
#include <deque>

#include "lexer/Lexer.h"
#include "parser/Parser.h"
#include "../engine/storage/Layout/Storage.h"

/*
This class will be responsible for running the thread that intakes queries
and forwarding it onto the query pipeline
*/
class QueryEngine {

    Storage& storage;
    Parser parser;
    Lexer lexer;

    std::thread query;  
    std::array<std::thread, 3> workers;

    void listen();
    void handle();

    std::mutex mu;
    std::deque<std::string> queries;

    //3 concurrent queries max
    std::counting_semaphore<3> sem{0};

public:

    QueryEngine(Storage& s) : storage(s), parser(storage) {
        
        if(query.joinable()) {
            query.join();
        }

        for(auto& worker : workers) {
            if(worker.joinable()) {
                worker.join();
            }
        }

    }

    void start() {
        std::println("Starting QueryEngine thread");
        query = std::thread(&QueryEngine::listen, this);
        
        for(auto& worker : workers) {
            worker = std::thread(&QueryEngine::handle, this);
        }

    }

};


