#include "../../deps/catch/catch.hpp"
#include "../../deps/mongoose/mongoose.h"

#include "../vaccine.h"

#include "http_client.h"


TEST_CASE("Accessing echo thru web service", "[echo]") {
  static const char * s_url = "http://127.0.0.1:8000/api/echo";

  // make sure we're running
  for (auto i = 0; vaccine::state != vaccine::mg_state::RUNNING || i < 10 ; i++)
    usleep(1000);

  SECTION("Simple http invocation to test respone body") {
   
    http_request(s_url, NULL, [] (struct http_message *hm) {

      CHECK( std::string(hm->body.p, hm->body.len)  == "HELO");
      CHECK( hm->resp_code == 200 );

      });

  }

  SECTION("echo is registred as callback") {
    CHECK( vaccine::registered_handlers().find( "echo" ) != vaccine::registered_handlers().end() );
  }
}
