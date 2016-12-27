#include "../../deps/catch/catch.hpp"

#include <string>
#include <vector>

#include "../../deps/mongoose/mongoose.h"

#include "../utils.hpp"

static void inline check_splitted_array(std::vector<std::string> const & ret) {
      CHECK( ret.size() == 3);
      CHECK( ret[0] == "foo");
      CHECK( ret[1] == "bar");
      CHECK( ret[2] == "barfoo");
}

TEST_CASE( "Split URLs into array of directories", "[utils]") {

  SECTION("split string") {
    SECTION("starting with delim") {
      const char * test = "/foo/bar/barfoo";
      
      auto ret = vaccine::split(test, '/');

      check_splitted_array(ret);
    }

    SECTION("trailing delim") {
      const char * test = "foo/bar/barfoo/";
      
      auto ret = vaccine::split(test, '/');

      check_splitted_array(ret);
    }

    SECTION("double delim") {
      const char * test = "/foo//bar//barfoo/";
      
      auto ret = vaccine::split(test, '/');

      check_splitted_array(ret);
    }

    SECTION("regular case") {
      const char * test = "foo/bar/barfoo";
      
      auto ret = vaccine::split(test, '/');

      check_splitted_array(ret);
    }

    SECTION("nullptr as input") {
      auto ret = vaccine::split(nullptr);

      CHECK(ret.size() == 0);
    }

    SECTION("only delimiter") {
      auto ret = vaccine::split("/",'/');

      CHECK(ret.size() == 0);
    }
  }

  SECTION("Split mg_str") {
    mg_str test = MG_MK_STR("foo/bar/barfoo");

    SECTION("regular case") {
      auto ret = vaccine::split(test, '/');

      check_splitted_array(ret);
    }

    SECTION("null len mg_str") {
      test.len = 0;

      auto ret = vaccine::split(test, '/');

      CHECK(ret.size() == 0);
    }

    SECTION("mg_str.p is nullptr") {
      test.p = nullptr;
      test.len = 0;

      auto ret = vaccine::split(test, '/');

      CHECK(ret.size() == 0);
    }
  }
}

