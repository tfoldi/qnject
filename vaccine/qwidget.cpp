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
#if 0
using namespace brilliant;


// move to : vaccine/utils/address.h
namespace {
    std::string address_to_string( void* ptr )
    {
      std::stringstream stream;
      stream << std::hex << ptr << std::dec;
      return stream.str();
    }

}
//
// move to : vaccine/request/response.h
namespace {
    // Abstract response:
    // a response is any type that can write itselt into an mg_connection


    struct response_prototype_t {
      // the only requirement for a response object is to have an
      // operator()() method taking the connection and writes it to
      // the response
      bool operator()( mg_connection* conn );
    };


    struct json_response_t {
      int statusCode;
      nlohmann::json value;

      bool operator()( mg_connection* conn ) {
        vaccine::send_json( conn, value, statusCode );

        // anything is ok between 200 and 300
        return (statusCode >= 200 && statusCode <= 299);
      }
    };

}



// Misc QT helpers ----------------- --------------------------------------------

// move to : vaccine/utils/qobject-finders.h
namespace {

    // Helper to invoke a function on a qobject if it has the specified name
    template<typename Functor>
    inline void with_object(const char* objectName, int& statusCode, Functor fn) {
        statusCode = 404;

        for (QWidget* child : qApp->allWidgets()) {
            if (child && (child->objectName() == objectName ||
                          objectName == std::to_string((uintptr_t) child))) {
                statusCode = 200;
                fn(child);
            }
        }
    }



    // Helper to invoke a function on a qobject if it has the specified name
    template<typename Functor>
    inline json_response_t with_object_at_address(const std::string& addrStr, Functor fn) {

        for (QWidget* child : qApp->allWidgets()) {

            auto thisAddr = address_to_string(child);


            DLOG_F(WARNING, "checking QOBJET @ %s against %s", thisAddr.c_str(), addrStr.c_str() );

            // TODO: do proper uintptr_t to uintptr_t comparison here
            if (child && (addrStr == address_to_string(child))) {
                return { 200, fn(child) };
            }
        }

        return { 404, std::string("Cannot find object @ ") + addrStr };
    }

}
#endif
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
                                           {"objectKind",  "widget"},
                                           {"className",  child->metaObject()->className()},
                                           {"superClass", child->metaObject()->superClass()->className()}
                                          });
            }

            return resp;
        }



        // GRAB IMAGE ------------------------------------------

        bool grab_qwidget_image(const Request& r, std::string object_address) {
            LOG_SCOPE_FUNCTION(INFO);

            int statusCode = 200;

            // // TODO: implement content-type type check
            qnject::with_object(object_name.c_str(), statusCode, [&](QWidget* obj) {
                QByteArray bytes;
                QBuffer buffer(&bytes);
                buffer.open(QIODevice::WriteOnly);
                obj->grab().save(&buffer, "PNG");

                mg_send_head(r.connection, 200, bytes.length(), "Content-Type: image/png");
                mg_send(r.connection, bytes.constData(), bytes.length());
            });

            if (statusCode == 200) {
                return true;
            }
            // we didn't find widget like this
            nlohmann::json resp;
            resp["error"] = "Widget not found";
            vaccine::send_json(r.connection, resp, statusCode);
            return false;
        }


        // handler: show QWidget

        // SET PROPERTIES ------------------------------------------

        bool set_qwidget(const Request& r, QObject* obj) {
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

            return true;
        }




        // CLICK ------------------------------------------

        bool click_qwidget(const Request& r, QObject* obj) {
            LOG_SCOPE_FUNCTION(INFO);

            auto* btn = qobject_cast<QAbstractButton*>(obj);
            if (btn != nullptr) {
                btn->click();
                return true;
            }

            return false;
        }

        using namespace brilliant::route;
        using namespace qnject::api;

        const auto qWidgetPath =
          prefix("api",
              prefix("qwidgets", any_of(

                  // leaf() only triggers if its the last entry in a path

                  // GET /api/qwidgets
                  leaf(get(handler(wrap_json(list_qwidgets)))),


                  // GET /api/qwidgets/grab/<OBJECT-NAME>
                  prefix("grab", handler("object-name", grab_qwidget_image)),


                  // use the same handlers as qobject_handler, but on a single, per-object
                  // base.
                  prefix("by-address", any_of(
                      prefix("click", qobject_at_address_handler(click_qwidget)),
                      prefix("grab", handler("object-address", grab_qwidget_image)),

                      get(qobject_at_address_handler(show_qwidget)),
                      post(qobject_at_address_handler(set_qwidget)),
                      handler(method_not_found)
                      )),

                  prefix("by-name", any_of(
                        get(qobject_at_address_handler(show_qwidget)),
                        post(qobject_at_address_handler(set_qwidget)),
                        handler(method_not_found)
                        )),


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
