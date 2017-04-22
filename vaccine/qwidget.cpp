#include "qnject_config.h"

#if HAVE_QT5CORE && HAVE_QT5WIDGETS

#include <dlfcn.h>
#include <map>
#include <vector>
#include <QApplication>
#include <QList>
#include <QObject>
#include <QWidget>
#include <QBuffer>
#include <QByteArray>
#include <QMetaObject>
#include <QMetaProperty>
#include <QAction>
#include <QMenu>
#include <QAbstractButton>

#include "../deps/loguru/loguru.hpp"
#include "../deps/json/json.hpp"

#include "utils.hpp"
#include "vaccine.h"
#include "qwidget.json.h"

#include "request.h"
#include "http-handlers/base.h"

// REQUEST / RESPONSE ----------------------------------------------------------

using brilliant::Request;
using qnject::address_to_string;
//
// Method / Signal / Slot metadata --------------------------------------------

namespace {
    using std::string;
    using std::vector;



    struct QObjectArg {
        string type;
        string name;
    };


    struct QObjectMethod {
        // PUBLIC, PRIVATE, PROTECTED
        string accessKind;

        // SIGNAL, SLOT, METHOD, COSTRUCTOR
        string type;

        string name;

        // The signature of the method
        string signature;

    };


    // Converts a QObject method type to its string repr
    string method_type_to_string(QMetaMethod::MethodType t) {
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
    string access_kind_to_string(QMetaMethod::Access t) {
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

    string qbyte_array_to_std_string(const QByteArray& bs) {
        return QString::fromLatin1(bs).toStdString();
    }


    // vector of string.... efficiency can suck it for now
    vector<QObjectMethod> get_qobject_method_metadata(const QMetaObject* metaObject) {

        vector<QObjectMethod> methods;


        for (int i = metaObject->methodOffset(); i < metaObject->methodCount(); ++i) {
            // get the meta object
            const auto& meth = metaObject->method(i);
            // append the data
            methods.emplace_back(QObjectMethod {
                    access_kind_to_string(meth.access()),
                    method_type_to_string(meth.methodType()),
                    qbyte_array_to_std_string(meth.name()),
                    qbyte_array_to_std_string(meth.methodSignature()),
            });
        }

        return methods;
    }


    void add_qobject_method_to_response(nlohmann::json& resp, const QObjectMethod& meth) {
        nlohmann::json local;
        local["name"] = meth.name;
        local["type"] = meth.type;
        local["access"] = meth.accessKind;
        local["signature"] = meth.signature;
        resp["methods"].push_back(local);
    }


}



// -- HANDLERS


// SHOW QWIDGET -----------------------------------------------------

namespace {


    // predicate for returning
    bool isScriptCommand(const QAction* action) {
        return action->text() == "Script Command";
    }

    template<typename triggerIf>
    void trigger_script_command(QAction* action, triggerIf pred) {
        // XXX: move to tableau.cpp or PATCH maybe?
        if (pred(action))
            action->activate(QAction::Trigger);
    }


    void
    add_property_to_response(nlohmann::json& resp, const char* kind, const char* propertyName, const QVariant val) {
        resp["properties"].push_back({
                                             {"name", propertyName},
                                             {"value", qPrintable(val.toString())},
                                             {"type", kind}
                                     });
    }


    nlohmann::json show_qwidget(const Request& r, QObject* obj) {
        nlohmann::json resp;
        const QMetaObject* metaObject = obj->metaObject();

        // Add regular properties
        for (auto i = 0; i < metaObject->propertyCount(); ++i) {
            add_property_to_response(resp, "meta", metaObject->property(i).name(), metaObject->property(i).read(obj));
        }


        // Add dynamic properties
        for (auto qbaPropertyName : obj->dynamicPropertyNames()) {
            auto propertyName = qbaPropertyName.constData();
            add_property_to_response(resp, "dynamic", propertyName, obj->property(propertyName));
        }

        // list actions
        for (auto action : ((QWidget*) obj)->actions()) {
            resp["actions"].push_back({{"text", qPrintable(action->text())},
                                       {"isVisible", action->isVisible()}
                                      });

            trigger_script_command(action, isScriptCommand);
        }

        // get the method information
        const auto methods = get_qobject_method_metadata(metaObject);

        // output the method information
        for (const auto& meth : methods) {
            add_qobject_method_to_response(resp, meth);
        }

        return resp;
    }
}


namespace {


    namespace api {



        // LIST ALL -------------------------------------------

        nlohmann::json list_qwidgets(const Request& r) {
            LOG_SCOPE_FUNCTION(INFO);

            nlohmann::json resp;
            // char buf[40];


            // sprintf(buf, "%p", qApp);

            resp["qApp"] = address_to_string(qApp);
            resp["appName"] = qPrintable(QApplication::applicationName());

            // All widgets
            for (QWidget* child : qApp->allWidgets()) {
                if (child == nullptr) continue;
                const QMetaObject* metaObject = child->metaObject();

                resp["widgets"].push_back({{"objectName", qPrintable(child->objectName())},
                                           {"address",    address_to_string(child)},
                                           {"parentName", child->parent() ? qPrintable(
                                                   child->parent()->objectName()) : ""},
                                           {"objectKind", "widget"},
                                           {"className",  child->metaObject()->className()},
                                           {"superClass", child->metaObject()->superClass()->className()}
                                          });
            }

            return resp;
        }

        using qnject::api::with_object_at_address;
        using brilliant::response::data_response_t;
        using qnject::json_response;
        using brilliant::response::error;
        using brilliant::response::fromContainer;
        using namespace brilliant;

        // GRAB IMAGE ------------------------------------------

        data_response_t grab_qwidget_image(const Request& r,
                                           QObject* obj) {

            LOG_SCOPE_FUNCTION(INFO);


            // TODO: implement content-type type check

            auto widget = qobject_cast<QWidget*>(obj);
            if (widget == nullptr)
                return error(404, "Object is not a QWidget");
            QByteArray bytes;
            QBuffer buffer(&bytes);
            buffer.open(QIODevice::WriteOnly);
            widget->grab().save(&buffer, "PNG");

            return response::fromData(200, "image/png", bytes.constBegin(), bytes.constEnd());

        }


        // handler: show QWidget

        // SET PROPERTIES ------------------------------------------

        data_response_t set_qwidget(const Request& r, QObject* obj) {
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

            return json_response(200, std::string("ok"));
        }




        // CLICK ------------------------------------------

        data_response_t click_qwidget(const Request& r, QObject* obj) {
            LOG_SCOPE_FUNCTION(INFO);

            auto btn = qobject_cast<QAbstractButton*>(obj);
            if (btn != nullptr) {
                btn->click();
                return json_response(200, std::string("clicked"));
            }

            return error(404, "Not a button");
        }

        using namespace brilliant::route;
        using namespace qnject::api;

        const auto byAddress =
                // use the same handlers as qobject_handler, but on a single, per-object
                // base.
                prefix("by-address", any_of(
                        prefix("click", qobject_at_address_handler(click_qwidget)),
                        prefix("grab", qobject_at_address_handler(grab_qwidget_image)),


                        get(qobject_at_address_json_handler(show_qwidget)),
                        post(qobject_at_address_handler(set_qwidget)),
                        handler(method_not_found)
                ));

        const auto qWidgetPath =
                prefix("api",
                       prefix("qwidgets", any_of(
                               // leaf() only triggers if its the last entry in a path

                               // GET /api/qwidgets
                               leaf(get(handler(wrap_json(list_qwidgets)))),


                               // GET /api/qwidgets/grab/<OBJECT-NAME>
//                               prefix("grab", handler("object-name", grab_qwidget_image)),

                               byAddress,
                               // use the same handlers as qobject_handler, but on a single, per-object
                               // base.
//                               prefix("by-address", any_of(
//                                       prefix("click", qobject_at_address_handler(click_qwidget)),
//                                       prefix("grab", handler("object-address", grab_qwidget_image)),
//
//                                       get(qobject_at_address_json_handler(show_qwidget)),
//                                       post(qobject_at_address_json_handler(set_qwidget)),
//                                       handler(method_not_found)
//                               )),

                               // prefix("by-name", any_of(
                               //       get(qobject_at_address_handler(show_qwidget)),
                               //       post(qobject_at_address_handler(set_qwidget)),
                               //       handler(method_not_found)
                               //       )),


                               handler(method_not_found)
                       )));
    }
}




// PUBLIC
//
namespace vaccine {

    void qwidget_handler(
            std::string& uri,
            struct mg_connection* nc,
            void* ev_data,
            struct http_message* hm) {

        using namespace brilliant::route;

        Request r = brilliant::request::fromBody(nc, hm);

        DLOG_F(INFO, "Serving URI: \"%s %s\" with post data >>>%.*s<<<",
               r.req["method"].get<std::string>().c_str(),
               uri.c_str(),
               (int) hm->body.len,
               hm->body.p);


        auto handled = api::qWidgetPath(make(r));

        // if we get here then we arent in the qwidget path.
        if (!handled) {
            api::method_not_found(r);
        }

    }

    __attribute__((constructor))
    static void initializer(void) {
        DLOG_F(INFO, "Register qwidget service");
        vaccine::register_callback("qwidgets", qwidget_handler, qwidget_json);
    }

}

#endif // HAVE QT5CORE && QT5WIDGES
