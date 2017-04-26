#pragma once

#include "json.hpp"
#include "loguru.hpp"
#include <QWidget>
#include <QMetaObject>
#include <QApplication>
#include "./base.h"
#include "../qwidget-json-helpers.h"

namespace qnject {
    namespace api {

        nlohmann::json qwidget_list(const Request& r) {
            using qnject::helpers::object_meta;

            nlohmann::json resp = {{"qApp",    address_to_string(qApp)},
                                   {"appName", qPrintable(QApplication::applicationName())},
                                   {"widgets", {}}
            };

            // All widgets
            for (QWidget* child : qApp->allWidgets()) {
                if (child == nullptr) continue;
                resp["widgets"].push_back(object_meta(child));
            }

            return resp;
        }

    }
}
