#pragma once

#include "loguru.hpp"
#include "json.hpp"
#include <QWidget>
#include <QMetaObject>
#include "./base.h"
#include "../qwidget-json-helpers.h"

namespace qnject {
    namespace api {

        data_response_t qwidget_update(const Request& r, QObject* obj) {
            LOG_SCOPE_FUNCTION(INFO);

            auto addr = address_to_string(obj);

            // set QWidget property (setPropert). Request type should be PATCH, but POST is accepted
            //
            // TODO: write tests
            DLOG_F(INFO, "Setting properties for object @ %s", addr.c_str());

            for (auto& prop : r.req["body"]["properties"]) {
                obj->setProperty(
                        prop["name"].get<std::string>().c_str(),
                        prop["value"].get<std::string>().c_str());
            }

            return json_response(200, qnject::helpers::qwidget_details(obj));
        }
    }
}

