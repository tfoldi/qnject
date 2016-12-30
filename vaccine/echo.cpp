#include "../config.h"
#include "vaccine.h"
#include "echo.json.h"

namespace vaccine {

  void echo_handler( 
      std::string & uri, 
      struct mg_connection *nc,  
      void *ev_data,            
      struct http_message *hm)
  {
    mg_send_head(nc, 200, 4, 
        "Access-Control-Allow-Origin: *\r\n"
        "Content-Type: text/plain");
    mg_send(nc,"HELO",4);
  }

  __attribute__((constructor))
    static void initializer(void) {
      printf("[%s] Register echo service\n", __FILE__);
      vaccine::register_callback("echo", echo_handler, echo_json);
    }

}
