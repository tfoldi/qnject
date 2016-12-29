#pragma once

#include <string>
#include <functional>

#include "../deps/mongoose/mongoose.h"
#include "../deps/json/json.hpp"

namespace vaccine {

  // vaccine's state
  enum mg_state {
    UNINITALIZED = 0,
    INITALIZING = 1,
    RUNNING = 2,
    SHUTDOWN_REQUESTED = 3,
    NOT_RUNNING = 4
  };
  
  // state of the injection. RUNNING means it accepts HTTP connections
  extern mg_state state;

  // vaccine's start thread function
  extern void start_thread(); 

  // callback type for registering 
  typedef std::function<void(
      std::string & uri, 
      struct mg_connection *nc, 
      void *ev_data,
      struct http_message *hm)> t_vaccine_api_handler;

  void register_callback(std::string handler, t_vaccine_api_handler function);
  
  const std::map<std::string, t_vaccine_api_handler> registered_handlers(); 

  void parse_request_body(struct http_message *hm, nlohmann::json & req);

  void send_json(struct mg_connection *nc, nlohmann::json & j, int statusCode = 200);
};

