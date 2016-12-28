#include "../../deps/catch/catch.hpp"
#include "../../deps/mongoose/mongoose.h"

#include "http_client.h"

TEST_CASE("Accessing echo thru web service", "[echo]") {
  static const char * s_url = "http://127.0.0.1:8000/api/echo";


  SECTION("Simple http invocation to test respone body") {
   
    http_request(s_url, NULL, [] (struct http_message *hm) {

      CHECK( std::string(hm->body.p, hm->body.len)  == "HELO");
      CHECK( hm->resp_code == 200 );

      });

  }
}
