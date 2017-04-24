#pragma once


#include "json.hpp"
#include <QObject.h>
#include <QMetaObject.h>
#include "./base.h"
#include "../qwidget-json-helpers.h"

namespace qnject {
    namespace api {

        nlohmann::json qwidget_meta(const Request& r, QObject* obj) {
            return qnject::helpers::qwidget_details(obj);
//            const QMetaObject* metaObject = obj->metaObject();

            return {{"object",     helpers::object_meta(obj)},
                    {"properties", helpers::object_properties_meta(obj)},
                    {"actions",    helpers::object_actions_meta(obj)},
                    {"methods",    helpers::object_methods_meta(obj)}};

//            resp.push_back({"qObject", helpers::object_meta(obj)});


//            // Add regular properties
//            for (auto i = 0; i < metaObject->propertyCount(); ++i) {
//                resp["properties"]
//                add_property_to_response(resp, "is-normal-property", metaObject->property(i).name(), metaObject->property(i).read(obj));
//            }
//
//
//            // Add dynamic properties
//            for (auto qbaPropertyName : obj->dynamicPropertyNames()) {
//                auto propertyName = qbaPropertyName.constData();
//                add_property_to_response(resp, "is-dynamic-property", propertyName, obj->property(propertyName));
//            }

            // list actions
//            for (auto action : ((QWidget*) obj)->actions()) {
//                resp["actions"].push_back({{"text", qPrintable(action->text())},
//                                           {"isVisible", action->isVisible()}
//                                          });
//
//            }

            // get the method information
//            const auto methods = get_qobject_method_metadata(metaObject);

            // output the method information
//            for (const auto& meth : methods) {
//                add_qobject_method_to_response(resp, meth);
//            }

//            return widget;
        }

    }
}