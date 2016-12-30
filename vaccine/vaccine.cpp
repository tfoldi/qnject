#include "../config.h"
#include <stdio.h>
#include <string>
#include <map>

#include "../deps/mongoose/mongoose.h"
#include "../deps/json/json.hpp"
#include "utils.hpp"

#include "vaccine.h"

namespace vaccine {

  // vaccine's state. read and write carefully 
  mg_state state = UNINITALIZED;

  // default port, can be override by VACCINE_HTTP_PORT env variable 
  static const char *s_default_http_port = "8000";

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
        {"basePath", "/api"},
        {"paths", nlohmann::json({}) }   
        });

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

  // check if an mg_str starts with an another mg_str. 
  static int has_prefix(const struct mg_str *uri, const struct mg_str *prefix) {
    return uri->len > prefix->len && memcmp(uri->p, prefix->p, prefix->len) == 0;
  }

  // get parsed json object from a http PUT/POST data req
  void parse_request_body(struct http_message *hm, nlohmann::json & req)
  {
    if ( hm->body.len > 0 ) {
      std::string s;
      s.assign(hm->body.p, hm->body.len);
      try {
        req = nlohmann::json::parse(s);
      } catch (std::exception & ex ) {
        printf("Request body: %s\n", ex.what());
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
    static const struct mg_str api_prefix = MG_MK_STR("/api/");
    static const struct mg_str swagger_json_uri = MG_MK_STR("/swagger.json");

    switch (ev) {
      case MG_EV_HTTP_REQUEST:
        if (has_prefix(&hm->uri, &api_prefix) && hm->uri.len - api_prefix.len > 0) {
          // API request
          std::string uri(hm->uri.p + api_prefix.len, hm->uri.len - api_prefix.len);
          std::string handler = get_until_char(uri,'/');
         
          // call registred handler
          printf("Request is %s\n", uri.c_str() );

          if ( s_uri_handlers.count(handler) == 1 ) {
            try {
              s_uri_handlers[handler](uri,nc,ev_data,hm);
            } catch (std::exception & ex) {
              mg_http_send_error(nc, 500, ex.what() );
            } catch (...) {
              mg_http_send_error(nc, 500, "Unknwown error (exception)" );
            }
          }
          else 
            mg_http_send_error(nc, 404, "Handler not registred");
        } else if (has_prefix(&hm->uri, &swagger_json_uri) ) {
          printf("I'm here\n");
          send_json(nc, s_swagger_json, 200);
        } else {
          // static web shit
          mg_serve_http(nc, hm, s_http_server_opts); 
        }
        break;
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
    printf("Starting vaccine HTTP REST API server on port %s\n", http_port);

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

    for (auto i = 0; vaccine::state != vaccine::mg_state::RUNNING || i * poll_sleep < usecs || usecs == -1 ; i++)
      usleep(poll_sleep);
  }
}
