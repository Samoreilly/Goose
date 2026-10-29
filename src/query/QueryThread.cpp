
#include "QueryEngine.h"
#include "parser/Parser.h"


void QueryEngine::listen() {

   while(true) {

      //blocking http request or ws connection
      std::string query {"GET * MSFT BETWEEN 01-12-2002..14-10-2013"};

      {
         std::lock_guard<std::mutex> lock{mu};
         queries.emplace_back(std::move(query));
      }

      sem.release();
   }

}

void QueryEngine::handle() {

   while(true) {

      sem.acquire();

      std::string query;

      {
         std::lock_guard<std::mutex> lock(mu);
         query = queries.front();
         queries.pop_front();
      }

      Query lexed_query = lexer.lex(query);

      parser.parse(std::move(lexed_query));


   }
}

