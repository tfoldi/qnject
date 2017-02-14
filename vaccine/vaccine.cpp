#include "qnject_config.h"
#include <stdio.h>
#include <string>
#include <map>

#define LOGURU_IMPLEMENTATION 1
#include "../deps/loguru/loguru.hpp"

#include "../deps/mongoose/mongoose.h"
#include "../deps/json/json.hpp"

#include "utils.hpp"
#include "vaccine.h"

#define VACCINE_API_PREFIX "/api"
#define VACCINE_SWAGGER_JSON "/swagger.json"

// TODO: refactor me to something like HTTP_HELPERS
namespace {
  using namespace vaccine;


  // declare these strings, so const char*-s wont get converted to std::string for every call
  const std::string PREFIX_VACCINE_API(VACCINE_API_PREFIX);
  const std::string PREFIX_VACCINE_SWAGGER_JSON(VACCINE_SWAGGER_JSON);


  // Checks if a string starts with another string
  bool starts_with( const std::string& s, const std::string& prefix ) {
    // prefix too long
    if (prefix.size() > s.size()) return false;
    return strncmp( s.c_str(), prefix.c_str(), prefix.size() ) == 0;
  }

  // Checks if an incoming URL is actually valid
  bool is_uri_valid(struct http_message* hm) {
    return (hm->uri.p != nullptr) && (hm->uri.len > 0);
  }


  // Checks if a request matches the vaccine url prefix
  bool uri_matches_vaccine_api(const std::string& uri) {
    return starts_with(uri, PREFIX_VACCINE_API);
  }

  // Checks if a request matches the swagger json route
  bool uri_matches_swagger_json(const std::string& uri) {
    return starts_with(uri, PREFIX_VACCINE_SWAGGER_JSON);
  }


  // Takes a URI and tries to get the handler name from it
  std::string get_handler_name(const std::string& uri) {
    // TODO: vector of string for a single element?
    return split( uri.c_str(), '/' )[1];
  }

  // Returns the path after VACCINE_API_PREFIX
  std::string get_vaccine_api_path( const std::string uri ) {
    static const size_t API_PREFIX_LEN = strlen(VACCINE_API_PREFIX);
    // Get the actual API path (if prefix is "/api" "/api/" should be removed
    // TODO: refactor the handlers to have paths beginning with slash, then remove this + 1
    return uri.substr( API_PREFIX_LEN + 1, std::string::npos);
  }
}

namespace vaccine {

  // vaccine's state. read and write carefully
  mg_state state = UNINITALIZED;

  // default port, can be override by VACCINE_HTTP_PORT env variable
  static constexpr auto s_default_http_port = "8000";

  // options for our http server.
  // XXX: this could be local var I guess using user_data
  static struct mg_serve_http_opts s_http_server_opts;

  // map handler with their callbacks
  static std::map<std::string, t_vaccine_api_handler> s_uri_handlers;

  // swagger service description
  static nlohmann::json s_swagger_json =
    nlohmann::json({
        {"swagger", "2.0"},
        {"info", {
          { "version", "0.1.0"},
          {"title", "qnject - injected in-process HTTP server"}
          }
        },
        {"basePath", VACCINE_API_PREFIX },
        {"paths", nlohmann::json({}) }
        });

  // return with swagger_json file
  const nlohmann::json swagger_json() { return s_swagger_json; }

  // simply push the callback function into our big map of handlers
  // this function is called from our handler "plugins"
  void register_callback(std::string handler, t_vaccine_api_handler function,
      const unsigned char * swagger_spec)
  {
    s_uri_handlers[handler] = function;
    if (swagger_spec) {
      // Add handler to our swagger definition:
      // /<handler -> specification from json.h file
      s_swagger_json["paths"].push_back(
          { "/" + handler, nlohmann::json::parse(swagger_spec)} );
    }
  }

  // const getter for the registred handlers
  const std::map<std::string, t_vaccine_api_handler> registered_handlers() {
    return s_uri_handlers;
  };

  // get parsed json object from a http PUT/POST data req
  void parse_request_body(struct http_message *hm, nlohmann::json & req)
  {
    if ( hm->body.len > 0 ) {
      std::string s(hm->body.p, hm->body.len);
      try {
        req = nlohmann::json::parse(s);
      } catch (std::exception & ex ) {
        DLOG_F(ERROR, "Request body: %s", ex.what());
      }
    }
  }

  // send json response to the client
  void send_json(struct mg_connection *nc, nlohmann::json & j, int statusCode)
  {
    static const auto headers =
      "Access-Control-Allow-Origin: *\r\n"
      "Access-Control-Allow-Headers: Origin, X-Requested-With, Content-Type, Accept\r\n"
      "Content-Type: application/json";

    std::string d = j.dump(2);

    mg_send_head(nc, statusCode, d.length(), headers);
    mg_send(nc,d.c_str(),d.length());
  }

  // this is our big fat HTTP event handler
  //
  // if URI starts with /api then it calls the matching handler
  // else tries to serve the static content piece
  //
  // TODO: refactor, move out HTTP_REQUEST handling and use split string
  // instead of has prefix and other mg_str crap
  static void ev_handler(struct mg_connection *nc, int ev, void *ev_data) {
    struct http_message *hm = (struct http_message *) ev_data;
    std::string uri;
    // std::vector<std::string> v_url;

    switch (ev) {
      case MG_EV_HTTP_REQUEST: {

        // check the validity of the url
        if (!is_uri_valid(hm)) {
          DLOG_F(INFO, "Got empty URL.");
          mg_http_send_error(nc, 204, "No content" );
          break;
        }

        // check if we are on the API path. This should be safe at this point
        uri.assign(hm->uri.p, hm->uri.len);


        // check if the URL is for the swagger json
        if (uri_matches_swagger_json(uri)) {
            DLOG_F(INFO, "Downloading " VACCINE_SWAGGER_JSON);
            send_json(nc, s_swagger_json, 200);
            break;
        }


        // if its not the swagger and not an API call, then server
        // anything static.
        // TODO: check for POST, only GETs should work
        if (!uri_matches_vaccine_api(uri)) {
          // static web shit
          mg_serve_http(nc, hm, s_http_server_opts);
          break;
        }

        // Get the name of the handler
        std::string handler = get_handler_name(uri);

        DLOG_SCOPE_F(INFO, "API request: '%.*s %s' => %s",
            (int)hm->method.len, hm->method.p, uri.c_str(), handler.c_str());

        // call registred handler
        if ( s_uri_handlers.count(handler) == 1 ) {
          try {
            // Since the handler expects an lvalue, we have to do this in two steps
            std::string api_path(get_vaccine_api_path(uri));
            s_uri_handlers[handler]( api_path, nc, ev_data, hm);
          } catch (std::exception & ex) {
            mg_http_send_error(nc, 500, ex.what() );
          } catch (...) {
            mg_http_send_error(nc, 500, "Unknwown error (exception)" );
          }
        } else {
          mg_http_send_error(nc, 404, "Handler not registred");
        }
        break;
    }
      default:
        break;
    }
  }

  // simply start our HTTP server thread
  void start_thread() {
    struct mg_mgr mgr;
    struct mg_connection *nc;
    const char * http_port = getenv("VACCINE_HTTP_PORT") == NULL ? s_default_http_port : getenv("VACCINE_HTTP_PORT");

    /* Open listening socket */
    mg_mgr_init(&mgr, NULL);
    nc = mg_bind(&mgr, http_port, ev_handler);
    mg_set_protocol_http_websocket(nc);

    /* set listing host as baseuri */
    s_swagger_json["host"] = std::string("localhost:") + http_port;

    /* mount UI. TODO: search for the right path */
    s_http_server_opts.document_root = "./elm-street";

    /* socket bind, document root set, ready to serve */
    state = mg_state::RUNNING;
    DLOG_F(INFO, "Starting vaccine HTTP REST API server on port %s", http_port);

    /* Run event loop until signal is received */
    while (state == mg_state::RUNNING) {
      mg_mgr_poll(&mgr, 1000);
    }

    /* Cleanup */
    mg_mgr_free(&mgr);
    state = mg_state::NOT_RUNNING;
  }

  // waits until vaccine is up and runnig. optional timeout in ms, -1 infinity
  void wait_until_vaccine_is_running(int usecs)
  {
    // make sure we're running
    static const auto poll_sleep = 1000;

    if (vaccine::state > vaccine::mg_state::RUNNING)
      throw std::runtime_error("vaccine already stopped");

    for (auto i = 0; vaccine::state != vaccine::mg_state::RUNNING || i * poll_sleep < usecs || usecs == -1 ; i++)
      usleep(poll_sleep);

    if (vaccine::state != vaccine::mg_state::RUNNING)
      throw std::runtime_error("timeout: wait_until_vaccine_is_running");
  }
}
