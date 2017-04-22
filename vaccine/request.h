#pragma once

#include <vector>
#include <string>


namespace brilliant {

    enum Method { GET, POST };

    struct Request {
        struct mg_connection* connection;

        nlohmann::json req;

        std::vector<std::string> uri;
        Method method;
    };



    // CONSTRUCTION -----------------------------------


    namespace request {


        // Creates a new request from the given http command
        Request fromBody(struct mg_connection* conn, struct http_message* hm) {
            nlohmann::json req;
            // parse the request
            vaccine::parse_request_body( hm, req );
            Method method = GET;
            if (vaccine::is_request_method(hm, "POST")) { method = POST; }
            std::string uri = req["uri"];
            return { conn, std::move(req), vaccine::split(uri.c_str(), '/'), method };
        }


    }





    namespace route {

      struct Route {
        const Request& request;
        // index of the head in the url bits
        size_t headIndex;
      };


      // Creates a new route
      Route make( const Request& req ) { return Route { req, 0 }; }

      // Increments the head without any checks
      Route next( Route r ) { r.headIndex += 1; return r; }

      // Returns true if the route has at least n components
      bool must_have_minimum_size( size_t n, const Route& r ) {
          return (r.request.uri.size() - r.headIndex) >= n;
      }


      // Returns the current path bit or an empty string if we are past
      // the end
      std::string head( const Route& r ) {
        return must_have_minimum_size(1, r)
          ? r.request.uri[r.headIndex]
          : "";
      }

      // list_objects = handle_with(list_objects_fn) -------------------


      // Fn will be called with the request, not the route
      template <typename Fn>
      struct handler_t {
          const Fn fn;

          bool operator()(const Route& r) const {
            return fn(r.request);
          }
      };

      template <typename Fn>
      handler_t<Fn> handler(Fn fn) {
          return { fn };
      }

      // URL PARAMETER -----------------------------------------------

      template <typename Fn>
      struct url_param_t {
          const Fn fn;

          bool operator()(const Route& r) const {
            return must_have_minimum_size(1,r) && fn(r.request, head(r));
          }
      };

      template <typename Fn>
      struct url_param2_t {
          const Fn fn;

          bool operator()(const Route& r) const {
            return must_have_minimum_size(1,r) && fn(r.request, head(r), head(next(r)));
          }
      };


      template <typename Fn>
      url_param_t<Fn> handler(const char* p0, Fn fn) {
        return { fn };
      }

      template <typename Fn>
      url_param2_t<Fn> handler(const char* p0, const char* p1, Fn fn) {
        return { fn };
      }
      // list_objects = leaf(handle_with(list_fn)) -------------------


      // call A if we are in a leaf route
      template <typename A>
      struct _leaf_t {
          const A a;

          bool operator()(const Route& r) const {
            return (r.headIndex == r.request.uri.size() ) && a(r);
          }
      };

      template <typename A>
      _leaf_t<A> leaf(A a) {
          return { a };
      }


      // foo = prefix("foo") => 'foo' routes --------------------------

      struct str_t {
        const std::string prefix;

        bool operator()(const Route& r) const {
          return must_have_minimum_size(1, r)
              && (r.request.uri[r.headIndex] == prefix);
        }
      };


      str_t str(std::string v) {
        return { v };
      }

      // foo = get("foo")    -------------------------
      // foo = post("foo")   -------------------------
      // foo = put("foo")    -------------------------
      // foo = patch("foo")  -------------------------

      template <typename A>
      struct method_t {
        const Method m;
        const A a;


        bool operator()(const Route& r) const {
          return (r.request.method == m) && a(r);
        }
      };


      template <typename A>
      method_t<A> get(A a) { return { GET, a }; }

      template <typename A>
      method_t<A> post(A a) { return { POST, a }; }




      // concat(foo,bar) => 'foo/bar' routes --------------------------


      template <typename Left, typename Right>
      struct concat_t {
        const Left left;
        const Right right;

        bool operator()(const Route& r) const {
          return must_have_minimum_size(1, r)
              && left(r)
              && right( next(r) );
        }
      };



      template <typename Left, typename Right>
      concat_t<Left, Right> concat( Left l, Right r ) {
        return { l, r };
      }

      template <typename Right>
      concat_t<str_t, Right> prefix(std::string v, Right r) {
        return { str_t { v }, r };
      }


      // any_of([foo,bar]) => 'foo' or 'bar' routes --------------------


      // unary implementation
      template <typename A0, typename A1>
      struct any_of_2_t {
        const A0 a0;
        const A1 a1;

        bool operator()(const Route& r) const {
          return a0(r) || a1(r);
        }
      };

      // n-ary aliases

      template <typename A0, typename A1, typename A2>
      using any_of_3_t = any_of_2_t<A0, any_of_2_t<A1,A2> >;

      template <typename A0, typename A1, typename A2, typename A3>
      using any_of_4_t = any_of_2_t<any_of_2_t<A0,A1>, any_of_2_t<A2,A3> >;

      template <typename A0, typename A1, typename A2, typename A3, typename A4>
      using any_of_5_t = any_of_2_t<any_of_2_t<A0,A1>, any_of_3_t<A2,A3,A4> >;

      template <typename A0, typename A1, typename A2, typename A3, typename A4, typename A5>
      using any_of_6_t = any_of_3_t< any_of_2_t<A0,A1>, any_of_2_t<A2,A3>, any_of_2_t<A4,A5> >;

      template <typename A0, typename A1, typename A2, typename A3, typename A4, typename A5, typename A6>
      using any_of_7_t = any_of_2_t< any_of_4_t<A0,A1,A2,A3>, any_of_3_t<A4,A5,A6> >;

      template <typename A0, typename A1, typename A2, typename A3, typename A4, typename A5, typename A6, typename A7>
      using any_of_8_t = any_of_2_t< any_of_4_t<A0,A1,A2,A3>, any_of_4_t<A4,A5,A6,A7> >;

      template <typename A0, typename A1, typename A2, typename A3, typename A4, typename A5, typename A6, typename A7, typename A8>
      using any_of_9_t = any_of_2_t< any_of_4_t<A0,A1,A2,A3>, any_of_5_t<A4,A5,A6,A7,A8> >;

      template <typename A0, typename A1, typename A2, typename A3, typename A4, typename A5, typename A6, typename A7, typename A8, typename A9>
      using any_of_10_t = any_of_3_t< any_of_4_t<A0,A1,A2,A3>, any_of_4_t<A4,A5,A6,A7>, any_of_2_t< A8, A9> >;

      // polymorphic implementataions of anyoif

      template <typename A0, typename A1>
      any_of_2_t<A0,A1> any_of( A0 a0, A1 a1 ) {
        return { a0, a1 };
      }

      template <typename A0, typename A1, typename A2>
      any_of_3_t<A0,A1,A2> any_of( A0 a0, A1 a1, A2 a2 ) {
        return { a0, {a1, a2} };
      }

      template <typename A0, typename A1, typename A2, typename A3>
      any_of_4_t<A0,A1,A2,A3> any_of( A0 a0, A1 a1, A2 a2, A3 a3 ) {
        return { {a0, a1}, {a2, a3} };
      }

      template <typename A0, typename A1, typename A2, typename A3, typename A4>
      any_of_5_t<A0,A1,A2,A3,A4> any_of( A0 a0, A1 a1, A2 a2, A3 a3, A4 a4 ) {
        return { {a0, a1}, {a2, a3, a4} };
      }

      template <typename A0, typename A1, typename A2, typename A3, typename A4, typename A5>
      any_of_6_t<A0,A1,A2,A3,A4,A5> any_of( A0 a0, A1 a1, A2 a2, A3 a3, A4 a4, A5 a5 ) {
        return { any_of(a0, a1), any_of(a2, a3), any_of(a4, a5) };
      }

      template <typename A0, typename A1, typename A2, typename A3, typename A4, typename A5, typename A6>
      any_of_7_t<A0,A1,A2,A3,A4,A5, A6> any_of( A0 a0, A1 a1, A2 a2, A3 a3, A4 a4, A5 a5, A6 a6 ) {
        return { any_of(a0, a1, a2, a3), any_of(a4, a5, a6) };
      }

//       template <typename A0, typename A1, typename A2, typename A3, typename A4, typename A5, typename A6, typename A7>
//       any_of_8_t<A0,A1,A2,A3,A4,A5> any_of( A0 a0, A1 a1, A2 a2, A3 a3, A4 a4, A5 a5, A6 a6, A7 a7) {
//         return { any_of(a0, a1), any_of(a2, a3), any_of(a4, a5) };
//       }

//       template <typename A0, typename A1, typename A2, typename A3, typename A4, typename A5, typename A6, typename A7, typename A8>
//       any_of_9_t<A0,A1,A2,A3,A4,A5> any_of( A0 a0, A1 a1, A2 a2, A3 a3, A4 a4, A5 a5, A6 a6, A7 a7, A8 a8) {
//         return { any_of(a0, a1), any_of(a2, a3), any_of(a4, a5) };
//       }

//       template <typename A0, typename A1, typename A2, typename A3, typename A4, typename A5, typename A6, typename A7, typename A8, typename A9>
//       any_of_10_t<A0,A1,A2,A3,A4,A5> any_of( A0 a0, A1 a1, A2 a2, A3 a3, A4 a4, A5 a5, A6 a6, A7 a7, A8 a8, A9 a9 ) {
//         return { any_of(a0, a1), any_of(a2, a3), any_of(a4, a5) };
//       }






    }



}


