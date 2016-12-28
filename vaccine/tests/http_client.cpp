#include <string>
#include <functional>
#include <stdexcept>

#include "../../deps/mongoose/mongoose.h"

typedef struct http_user_data {
  int err_code;
  bool done;
  std::function<void(struct http_message *hm)> callback;
} http_user_data;

static void ev_handler(struct mg_connection *nc, int ev, void *ev_data) {
  struct http_message *hm = (struct http_message *) ev_data;
  int connect_status;
  http_user_data * user_data = (http_user_data *)nc->user_data;

  switch (ev) {
    case MG_EV_CONNECT:
      user_data->err_code = *(int *) ev_data;
      if (connect_status != 0) {
        printf("Error connecting  %s\n",  strerror(connect_status));
      }
      break;
    case MG_EV_HTTP_REPLY:
      printf("Got reply:\n%.*s\n", (int) hm->body.len, hm->body.p);

      nc->flags |= MG_F_SEND_AND_CLOSE;
      user_data->done = true;

      user_data->callback(hm);
      break;
    case MG_EV_CLOSE:
      if (!user_data->done ) {
        user_data->done = true;
      };
      break;
    default:
      break;
  }
}

void http_request(const char * url, const char * data, std::function<void(struct http_message *hm)> callback) 
{
  struct mg_mgr mgr;
  struct mg_connection *nc;
  struct mg_connect_opts opts;
  struct http_user_data user_data {0, false, callback};

  mg_mgr_init(&mgr, NULL);

  memset(&opts, 0, sizeof(opts));
  opts.user_data = &user_data;

  nc = mg_connect_http_opt(&mgr, ev_handler, opts, url, NULL, data);
  mg_set_protocol_http_websocket(nc);

  printf("Starting RESTful client against %s\n", url);
  while (!user_data.done) {
    mg_mgr_poll(&mgr, 1000);
  }
  mg_mgr_free(&mgr);

  if (user_data.err_code != 0)
    throw std::runtime_error( strerror(user_data.err_code) );

}

