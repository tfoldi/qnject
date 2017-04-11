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

// REQUEST / RESPONSE ----------------------------------------------------------

using namespace brilliant;




// Misc QT helpers ----------------- --------------------------------------------

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


    // Converts a pointer to a string
    std::string address_to_string(void* ptr) {
        return std::to_string((uintptr_t) ptr);
    }

}

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
            char buf[40];


            sprintf(buf, "%p", qApp);

            resp["qApp"] = buf;
            resp["appName"] = qPrintable(QApplication::applicationName());

            // All widgets
            for (QWidget* child : qApp->allWidgets()) {
                if (child == nullptr) continue;
                const QMetaObject* metaObject = child->metaObject();

                resp["widgets"].push_back({{"objectName", qPrintable(child->objectName())},
                                           {"address",    address_to_string(child)},
                                           {"parentName", child->parent() ? qPrintable(
                                                   child->parent()->objectName()) : ""},
                                           {"obectKind",  "widget"},
                                           {"className",  child->metaObject()->className()},
                                           {"superClass", child->metaObject()->superClass()->className()}
                                          });
            }

            return resp;
        }



        // GRAB IMAGE ------------------------------------------

        bool grab_qwidget_image(const Request& r, std::string object_name) {
            LOG_SCOPE_FUNCTION(INFO);

            int statusCode = 200;

            // // TODO: implement content-type type check
            with_object(object_name.c_str(), statusCode, [&](QWidget* obj) {
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


        using namespace route;

        // GENERIC JSON HANDLER --------------------------
        //

        template<typename Handler>
        struct json_handler_t {
            Handler fn;

            template<typename ...Args>
            bool operator()(const Request& r, Args ...args) const {
                // lvalue is needed here
                nlohmann::json ret = fn(r, args...);
                vaccine::send_json(r.connection, ret, 200);
                return true;
            }
        };

        template<typename Handler>
        json_handler_t<Handler> wrap_json(Handler h) {
            return {h};
        }


        // GENERIC QOBJECT HANDLER --------------------------
        //
        // Maps a single URL parameter to a list of QObjects with the name,
        // maps Handler on them and returns the concatenated response as JSON.


        // Wraps a QObject -> Json request
        template<typename Handler>
        struct qobject_handler_t {
            Handler fn;

            bool operator()(const Request& r, std::string object_name) const {
                // take a single URL parameter
                int statusCode = 200;
                nlohmann::json resp;

                DLOG_F(INFO, "Requesting data : name ='%s'", object_name.c_str());
                with_object(object_name.c_str(), statusCode, [&](QObject* obj) {
                    auto addr = address_to_string(obj);
                    DLOG_F(INFO, "Object found @ %s : name ='%s'", addr.c_str(), object_name.c_str());
                    resp.push_back({addr, fn(r, obj)});
                });

                // if error
                if (statusCode < 299) {
                    vaccine::send_json(r.connection, resp, statusCode);
                    return true;
                }

                // we didn't find widget like this
                nlohmann::json err = {{"error", "Widget not found"}};
//                resp["error"] = ;
                vaccine::send_json(r.connection, err, statusCode);
                return false;
            }
        };


        // Wraps a handler
        template<typename Fn>
        decltype(auto) qobject_handler(Fn fn) {
            return handler("object-name", qobject_handler_t<Fn>{fn});
        };



        // API -----------------------------------------------------
        //
        bool method_not_found(const Request& r) {
            std::string uri = r.req["uri"];
            DLOG_F(WARNING, "Unhandled request: %s", uri.c_str());

            nlohmann::json resp;
            resp["error"] = "Method not found";
            vaccine::send_json(r.connection, resp, 404);
            return true;
        }


        const auto qWidgetPath =
                prefix("api",
                       prefix("qwidgets", any_of(

                               // leaf() only triggers if its the last entry in a path

                               // GET /api/qwidgets
                               leaf(get(handler(wrap_json(list_qwidgets)))),


                               // GET /api/qwidgets/grab/<OBJECT-NAME>
                               prefix("grab", handler("object-name", grab_qwidget_image)),

                               // GET /api/qwidgets/click/<OBJECT-NAME>
                               prefix("click", qobject_handler(click_qwidget)),


                               //
                               // routes are evaluated depth-first, so put more generic to the end
                               //

                               // GET /api/qwidgets/<OBJECT-NAME>
                               get(qobject_handler(show_qwidget)),

                               // POST /api/qwidgets/<OBJECT-NAME>
                               post(qobject_handler(set_qwidget)),

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

        Request r = request::fromBody(nc, hm);

        DLOG_F(INFO, "Serving URI: \"%s %s\" with post data >>>%.*s<<<",
               r.req["method"].get<std::string>().c_str(),
               uri.c_str(),
               (int) hm->body.len,
               hm->body.p);


        auto handled = api::qWidgetPath(route::make(r));

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
