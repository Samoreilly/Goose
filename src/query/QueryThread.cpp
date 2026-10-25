
#include "QueryEngine.h"
#include <atomic>


void QueryEngine::listen() {

   while(true) {

      //http request or ws connection
      std::string query {"GET * MSFT BETWEEN 01-12-2002..14-10-2013"};

      {
         std::lock_guard<std::mutex> lock{mu};
         queries.emplace_back(query);
      }

      

   }

}

void QueryEngine::handle() {

   auto get_new_query = [&]() -> QueryHandling* {
      for(auto it = queries.rbegin(); it != queries.rend();it++) {
         bool expected {false};
         //changes it to true, if false in 1 atomic instruction
         if(it->handled.compare_exchange_strong(expected, true)) {
            return &*it;
         }
      }
      return nullptr;
   };

   while(true) {

      sem.acquire();
      
      auto& query_handler = queries.back();
      
      //true means its being handled
      if(query_handler.handled.load(std::memory_order_acquire)) {
         query_handler = get_new_query();
      }

      lexer.parse(query_handler.query);

      sem.release();
   }
}

