#include "../config.h"
#include <stdio.h>

#include "../deps/mongoose/mongoose.h"

bool vaccine_shutdown = false;

static const char *s_http_port = "8000";
static struct mg_serve_http_opts s_http_server_opts;
static const struct mg_str s_get_method = MG_MK_STR("GET");
static const struct mg_str s_put_method = MG_MK_STR("PUT");
static const struct mg_str s_delele_method = MG_MK_STR("DELETE");


static void ev_handler(struct mg_connection *nc, int ev, void *ev_data) {
  static const struct mg_str api_prefix = MG_MK_STR("/api/v1");
  struct http_message *hm = (struct http_message *) ev_data;
  struct mg_str key;

  switch (ev) {
    case MG_EV_HTTP_REQUEST:
#if 0
      if (has_prefix(&hm->uri, &api_prefix)) {
        key.p = hm->uri.p + api_prefix.len;
        key.len = hm->uri.len - api_prefix.len;
        if (is_equal(&hm->method, &s_get_method)) {
          db_op(nc, hm, &key, s_db_handle, API_OP_GET);
        } else if (is_equal(&hm->method, &s_put_method)) {
          db_op(nc, hm, &key, s_db_handle, API_OP_SET);
        } else if (is_equal(&hm->method, &s_delele_method)) {
          db_op(nc, hm, &key, s_db_handle, API_OP_DEL);
        } else {
          mg_printf(nc, "%s",
                    "HTTP/1.0 501 Not Implemented\r\n"
                    "Content-Length: 0\r\n\r\n");
        }
      } else {
      }
#endif        
        mg_serve_http(nc, hm, s_http_server_opts); /* Serve static content */
      break;
    default:
      break;
  }
}


void start_thread() {
  struct mg_mgr mgr;
  struct mg_connection *nc;
  int i;

  printf("Opening web server socket\n");

  /* Open listening socket */
  mg_mgr_init(&mgr, NULL);
  nc = mg_bind(&mgr, s_http_port, ev_handler);
  mg_set_protocol_http_websocket(nc);
  s_http_server_opts.document_root = "/tmp/a";

  /* Run event loop until signal is received */
  printf("Starting RESTful server on port %s\n", s_http_port);
  while (! vaccine_shutdown) {
    mg_mgr_poll(&mgr, 1000);
  }

  /* Cleanup */
  mg_mgr_free(&mgr);

  printf("Exiting on shutdown request");
}
