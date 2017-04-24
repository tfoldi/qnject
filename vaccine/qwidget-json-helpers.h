#pragma once
//
// Created by Miles Gibson on 24/04/17.
//
#include <QObject.h>
#include "json.hpp"
#include <string>
#include <sstream>

namespace qnject {
    namespace helpers {

        std::string address_to_string(void* ptr) {
            std::stringstream stream;
            stream << std::hex << ptr << std::dec;
            return stream.str();
        }

        // Get the generic metadata for an object
        nlohmann::json object_meta(QObject* child) {
            return {{"objectName", qPrintable(child->objectName())},
                    {"address",    address_to_string(child)},
                    {"parentName", child->parent() ? qPrintable(
                            child->parent()->objectName()) : ""},
                    {"objectKind", "widget"},
                    {"className",  child->metaObject()->className()},
                    {"superClass", child->metaObject()->superClass()->className()}
            };
        }


        // Get the generic metadata for an action
        nlohmann::json action_meta(QAction* a) {
            nlohmann::json actionMeta = {
                    {"isCheckable", a->isCheckable()},
                    {"isChecked",   a->isChecked()},
                    {"isEnabled",   a->isEnabled()},
                    {"isVisible",   a->isVisible()},
                    {"text",      qPrintable(a->text())},
                    {"toolTip",   qPrintable(a->toolTip())},
                    {"whatsThis", qPrintable(a->whatsThis())}};

            return {{"object", object_meta(a)},
                    {"action", actionMeta}};
        }


    }
}