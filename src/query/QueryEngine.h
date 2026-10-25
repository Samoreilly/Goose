#pragma once

#include <thread>
#include <print>
#include <array>
#include <vector>
#include <mutex>
#include <atomic>
#include <semaphore>
#include <string>

#include "lexer/Lexer.h"

struct QueryHandling {
    std::string query;
    std::atomic<bool> handled {false};

    QueryHandling(std::string q) : query(std::move(q)) {}

    //for vector resizing
    QueryHandling(QueryHandling&& other) noexcept 
        : query(std::move(other.query)), 
          handled(other.handled.load()) {}

    QueryHandling(const QueryHandling&) = delete;
    QueryHandling& operator=(const QueryHandling&) = delete;

};


/*
This class will be responsible for running the thread that intakes queries
and forwarding it onto the query pipeline
*/
class QueryEngine {

    Lexer& lexer;

    std::thread query;  
    std::array<std::thread, 3> workers;

    void listen();
    void handle();

    std::mutex mu;
    std::vector<QueryHandling> queries;

    std::counting_semaphore<3> sem{3};

public:

    QueryEngine(Lexer& l) : lexer(l) {
        
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


