#pragma once

#include <string>
#include <sstream>

namespace qnject {

  // TODO: move these and PODs out to a separate http-handlers/types.h
  using String = std::string;
  using Json = nlohmann::json;

  //
  //
  // move to : vaccine/utils/address.h
  String address_to_string( void* ptr )
  {
    std::stringstream stream;
    stream << std::hex << ptr << std::dec;
    return stream.str();
  }

  // Misc QT helpers ----------------- --------------------------------------------

  // move to : vaccine/utils/qobject-finders.h
  // namespace {

    // Helper to invoke a function on a qobject if it has the specified name
    template<typename Functor>
      inline void with_object(const char* objectName, int& statusCode, Functor fn) {
        statusCode = 404;

        for (QWidget* child : qApp->allWidgets()) {
          if (child && (child->objectName() == objectName ||
                objectName == std::to_string((uintptr_t) child))) {
            statusCode = 200;
            fn(child);
          }
        }
      }



}



namespace brilliant {
  //
  // move to : vaccine/request/response.h
  // ------------------------------------

  // Abstract response:
  // a response is any type that can write itselt into an mg_connection

  // TODO: move these and PODs out to a separate http-handlers/types.h

  struct response_prototype_t {
    // the only requirement for a response object is to have an
    // operator()() method taking the connection and writes it to
    // the response
    bool operator()( mg_connection* conn );
  };
}

namespace qnject {

  struct json_response_t {
    int statusCode;
    qnject::Json value;

    bool operator()( mg_connection* conn ) {
      vaccine::send_json( conn, value, statusCode );

      // anything is ok between 200 and 300
      return (statusCode >= 200 && statusCode <= 299);
    }
  };




}



// GENERIC HANDLERS
//
namespace qnject {
  namespace api {

        // using namespace brilliant::route;

        // for now use these, revisit later
        using Request = brilliant::Request;

        // GENERIC JSON HANDLER --------------------------
        //

        template<typename Handler>
        struct json_handler_t {
            Handler fn;

            template<typename ...Args>
            bool operator()(const Request& r, Args ...args) const {
                // lvalue is needed here
                Json ret = fn(r, args...);
                vaccine::send_json(r.connection, ret, 200);
                return true;
            }
        };

        template<typename Handler>
        json_handler_t<Handler> wrap_json(Handler h) {
            return {h};
        }


        // GENERIC QOBJECT HANDLER --------------------------
        //
        // Maps a single URL parameter to a list of QObjects with the name,
        // maps Handler on them and returns the concatenated response as JSON.


        // Wraps a QObject -> Json request
        template<typename Handler>
        struct qobject_handler_t {
            Handler fn;

            bool operator()(const Request& r, String object_name) const {
                // take a single URL parameter
                int statusCode = 200;
                Json resp;

                DLOG_F(INFO, "Requesting data for QObject by name ='%s'", object_name.c_str());
                with_object(object_name.c_str(), statusCode, [&](QObject* obj) {
                    auto addr = address_to_string(obj);
                    DLOG_F(INFO, "Object found @ %s : name ='%s'", addr.c_str(), object_name.c_str());
                    resp.push_back({addr, fn(r, obj)});
                });

                // if error
                if (statusCode < 299) {
                    vaccine::send_json(r.connection, resp, statusCode);
                    return true;
                }

                // we didn't find widget like this
                Json err = {{"error", "Widget not found"}};
//                resp["error"] = ;
                vaccine::send_json(r.connection, err, statusCode);
                return false;
            }
        };


        // Wraps a handler, we need to auto here to keep our sanity.
        template<typename Fn>
        decltype(auto) qobject_handler(Fn fn) {
            using namespace brilliant::route;
            return handler("object-name", qobject_handler_t<Fn>{fn});
        };


        // QOBJECT BY POINTER HANDLER BASE ---------------------------

        // Maps a single URL parameter to a single QObject with the url parameter as address,
        // maps Handler on them and returns the response as JSON.

        // Helper to invoke a function on a qobject if it has the specified name
        template<typename Functor>
          inline json_response_t with_object_at_address(const String& addrStr, Functor fn) {

            for (QWidget* child : qApp->allWidgets()) {

              auto thisAddr = address_to_string(child);

              // TODO: do proper uintptr_t to uintptr_t comparison here
              if (child && (addrStr == address_to_string(child))) {
                return { 200, fn(child) };
              }
            }

            return { 404, String("Cannot find object @ ") + addrStr };
          }


        // Wraps a QObject -> Json request
        template<typename Handler>
        struct qobject_by_address_handler_t {
            Handler fn;

            bool operator()(const Request& r, String object_address) const {

                DLOG_F(INFO, "Requesting object @ '%s'", object_address.c_str());
                auto response = with_object_at_address(object_address.c_str(), [&](QObject* obj) {
                    auto addr = address_to_string(obj);
                    return fn(r, obj);
                });

                return response(r.connection);
            }
        };


        // Wraps a handler
        template<typename Fn>
        decltype(auto) qobject_at_address_handler(Fn fn) {
            using namespace brilliant::route;
            return handler("object-address", qobject_by_address_handler_t<Fn>{fn});
        };




        // NOT FOUND :) -----------------------------------------------------
        //
        bool method_not_found(const Request& r) {
            String uri = r.req["uri"];
            DLOG_F(WARNING, "Unhandled request: %s", uri.c_str());

            Json resp;
            resp["error"] = "Method not found";
            vaccine::send_json(r.connection, resp, 404);
            return true;
        }


  }
}
