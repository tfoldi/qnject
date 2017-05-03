#include "../../../deps/catch/catch.hpp"

#include <string>
#include <QApplication>

#include "../../../deps/mongoose/mongoose.h"

#include "../../vaccine.h"

#include "../http_client.h"


using nlohmann::json;

TEST_CASE("QT Widget tests", "[qt]") {

  using namespace std::literals;
  // make sure we're running
  vaccine::wait_until_vaccine_is_running(10000us);

  SECTION("Get all objects") {
    static const char * s_url_app_properties = "http://127.0.0.1:8000/api/qwidgets";

    SECTION("Simple http invocation to test respone body") {

      http_request(s_url_app_properties, NULL, [] (struct http_message *hm) {
          REQUIRE( hm->resp_code == 200 );
          });
      http_request(s_url_app_properties, "", [] (struct http_message *hm) {
          CHECK( hm->resp_code == 404 );
          });
    }

    SECTION("Check contents of the result") {
      http_request(s_url_app_properties, NULL, [] (struct http_message *hm) {
          CHECK( hm->resp_code == 200 );
          
          json ret = json::parse( std::string(hm->body.p, hm->body.len));
          CHECK( ret["appName"].get<std::string>() == "qt_testapp" );
          CHECK( strtoll(ret["qApp"].get<std::string>().c_str(), NULL, 0) == (long long)qApp);
          CHECK( ret["widgets"].size() >= 10 );
          });

    }
  }

  SECTION("qobject is registred as callback") {
    CHECK( vaccine::registered_handlers().find( "qobject" ) != vaccine::registered_handlers().end() );
  }
}

