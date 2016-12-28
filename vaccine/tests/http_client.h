#pragma once

#include <functional>
#include "../../deps/mongoose/mongoose.h"

void http_request(const char * url, const char * data, std::function<void(struct http_message *hm)> callback);


