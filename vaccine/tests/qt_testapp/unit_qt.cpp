#include "../../../deps/catch/catch.hpp"
#include "../../../deps/mongoose/mongoose.h"

#include "../../vaccine.h"

#include "../http_client.h"

TEST_CASE("QT Widget tests", "[qt]") {
  static const char * s_url = "http://127.0.0.1:8000/api/qwidgets";

  // make sure we're running
  vaccine::wait_until_vaccine_is_running(10000);

  SECTION("Simple http invocation to test respone body") {
   
    http_request(s_url, NULL, [] (struct http_message *hm) {
        CHECK( hm->resp_code == 200 );
      });
    http_request(s_url, "", [] (struct http_message *hm) {
        CHECK( hm->resp_code == 404 );
      });

  }

  SECTION("qobject is registred as callback") {
    CHECK( vaccine::registered_handlers().find( "qobject" ) != vaccine::registered_handlers().end() );
  }
}

