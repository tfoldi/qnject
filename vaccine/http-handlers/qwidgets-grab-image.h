#pragma once

#include "json.hpp"
#include "loguru.hpp"
#include <QWidget>
#include <QApplication>
#include <QByteArray>
#include <QBuffer>
#include "./base.h"
#include "../qwidget-json-helpers.h"

namespace qnject {
    namespace api {

        data_response_t qwidgets_grab_image(const Request& r,
                                           QObject* obj) {
            using namespace brilliant;
            LOG_SCOPE_FUNCTION(INFO);


            auto widget = qobject_cast<QWidget*>(obj);
            if (widget == nullptr)
                return response::error(404, "Object is not a QWidget");

            QByteArray bytes;
            QBuffer buffer(&bytes);
            buffer.open(QIODevice::WriteOnly);
            // TODO: implement content-type type check
            widget->grab().save(&buffer, "PNG");

            return response::fromData(200, "image/png", bytes.constBegin(), bytes.constEnd());

        }

    }
}
