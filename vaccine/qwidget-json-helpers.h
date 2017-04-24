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

        // GENERIC ----------------------------------------

        std::string address_to_string(void* ptr) {
            std::stringstream stream;
            stream << std::hex << ptr << std::dec;
            return stream.str();
        }

        // QOBJECT ----------------------------------------

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

        // ACTIONS ----------------------------------------

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


        // Returns the action metadata for a QWidget
        nlohmann::json object_actions_meta(QObject* obj) {
            auto j = "[]"_json;

            auto o = qobject_cast<QWidget*>(obj);
            if (o == nullptr) {
                return {{"error", "Cannot cast QObject to QAction"}};
            }

            for (auto action : o->actions()) {
                j.push_back(action_meta(action));
            }
            return j;
        }


        // PROPERTIES ----------------------------------------

        // Gets the property metadata
        nlohmann::json property_meta(const std::string& source, const std::string& name, const QVariant& value) {
            return {{"name",   name},
                    {"value", qPrintable(value.toString())},
                    {"source", source}};
        }


        // Gets the property metadata
        nlohmann::json object_properties_meta(QObject* obj) {
            nlohmann::json properties = "[]"_json;
            // Add regular properties
            auto metaObject = obj->metaObject();

            for (auto i = 0; i < metaObject->propertyCount(); ++i) {
                properties.push_back(
                        property_meta("fromClass",
                                      metaObject->property(i).name(),
                                      metaObject->property(i).read(obj)));
            }


            // Add dynamic properties
            for (auto qbaPropertyName : obj->dynamicPropertyNames()) {
                auto propertyName = qbaPropertyName.constData();
                properties.push_back(
                        property_meta("fromInstance",
                                      propertyName,
                                      obj->property(propertyName)));
            }

            return properties;
        }

        // METHODS ----------------------------------------



        // Converts a QObject method type to its string repr
        inline std::string method_type_to_string(QMetaMethod::MethodType t) {
            switch (t) {
                case QMetaMethod::Signal:
                    return "signal";
                case QMetaMethod::Slot:
                    return "slot";
                case QMetaMethod::Method:
                    return "method";
                case QMetaMethod::Constructor:
                    return "constructor";
                default:
                    return "<UNKNONW METHOD TYPE>";
            }
        }

        // Converts a QObject method type to its string repr
        inline std::string access_kind_to_string(QMetaMethod::Access t) {
            switch (t) {
                case QMetaMethod::Public:
                    return "public";
                case QMetaMethod::Private:
                    return "private";
                case QMetaMethod::Protected:
                    return "protected";
                default:
                    return "<UNKNONW METHOD ACCESS>";
            }
        }


        inline nlohmann::json method_meta(QMetaMethod meth) {
            return {{"name",      qPrintable(meth.name())},
                    {"signature", qPrintable(meth.methodSignature())},
                    {"access", access_kind_to_string(meth.access())},
                    {"type",   method_type_to_string(meth.methodType())}};
        }


        inline nlohmann::json object_methods_meta(QObject* obj) {

            auto j = "[]"_json;

            auto metaObject = obj->metaObject();
            if (metaObject == nullptr) {
                return {{"error", "Cannot get metaObject() for QObject"}};
            }

            for (int i = metaObject->methodOffset(); i < metaObject->methodCount(); ++i) {
                // append the data
                j.push_back(method_meta(metaObject->method(i)));
            }

            return j;
        }

        nlohmann::json qwidget_details(QObject* obj) {
//            const QMetaObject* metaObject = obj->metaObject();

            return {{"object",     helpers::object_meta(obj)},
                    {"properties", helpers::object_properties_meta(obj)},
                    {"actions",    helpers::object_actions_meta(obj)},
                    {"methods",    helpers::object_methods_meta(obj)}};
        }

        // ALL THINGS FOR QOBJECT ----------------------------------------


    }
}