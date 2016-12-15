#pragma once

#include <string>
#include "../deps/mongoose/mongoose.h"
#include "../deps/json/json.hpp"

namespace vaccine {

  // shutdown flag for the service thread
  extern bool shutdown;

  // vaccine's start thread function
  extern void start_thread(); 

  // callback type for registering 
  typedef void (*t_vaccine_api_handler)(
      std::string & uri, 
      struct mg_connection *nc, 
      void *ev_data,
      struct http_message *hm);
  
  void register_callback(std::string handler, t_vaccine_api_handler function);

  void parse_request_body(struct http_message *hm, nlohmann::json & req);

  void send_json(struct mg_connection *nc, nlohmann::json & j, int statusCode = 200);
};

