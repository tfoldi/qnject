#pragma once


#include "json.hpp"
#include <QObject.h>
#include <QMetaObject.h>
#include "./base.h"
#include "../qwidget-json-helpers.h"

namespace qnject {
    namespace api {

        nlohmann::json qwidget_show(const Request& r, QObject* obj) {
            return qnject::helpers::qwidget_details(obj);
        }

    }
}