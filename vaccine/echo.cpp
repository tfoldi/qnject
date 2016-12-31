#include "../config.h"

#include "../deps/loguru/loguru.hpp"

#include "vaccine.h"
#include "echo.json.h"

namespace vaccine {

  void echo_handler( 
      std::string & uri, 
      struct mg_connection *nc,  
      void *ev_data,            
      struct http_message *hm)
  {
    DLOG_F(INFO, "Echo handler called");

    mg_send_head(nc, 200, 4, 
        "Access-Control-Allow-Origin: *\r\n"
        "Content-Type: text/plain");
    mg_send(nc,"HELO",4);
  }

  __attribute__((constructor))
    static void initializer(void) {
      DLOG_F(INFO, "Register echo service");
      vaccine::register_callback("echo", echo_handler, echo_json);
    }

}
