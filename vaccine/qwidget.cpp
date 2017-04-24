#include "qnject_config.h"

#if HAVE_QT5CORE && HAVE_QT5WIDGETS

#include <map>
#include <vector>


#include "../deps/loguru/loguru.hpp"
#include "../deps/json/json.hpp"

#include "utils.hpp"
#include "vaccine.h"
#include "qwidget.json.h"

#include "request.h"
#include "qwidget-json-helpers.h"
#include "http-handlers/base.h"
#include "http-handlers/menu-tree-get.h"
#include "http-handlers/menu-tree-trigger-action.h"
#include "http-handlers/qwidgets-list.h"
#include "http-handlers/qwidgets-show.h"
#include "http-handlers/qwidgets-update.h"
#include "http-handlers/qwidgets-grab-image.h"
#include "http-handlers/qwidgets-get-menubar.h"

// REQUEST / RESPONSE ----------------------------------------------------------

using brilliant::Request;
using qnject::address_to_string;
using qnject::helpers::object_meta;
//
// Method / Signal / Slot metadata --------------------------------------------




// -- HANDLERS



namespace {


    namespace api {




        // CLICK ------------------------------------------

//        data_response_t click_qwidget(const Request& r, QObject* obj) {
//            LOG_SCOPE_FUNCTION(INFO);
//
//            auto btn = qobject_cast<QAbstractButton*>(obj);
//            if (btn != nullptr) {
//                btn->click();
//                return json_response(200, std::string("clicked"));
//            }
//
//            return error(404, "Not a button");
//        }

        using namespace brilliant::route;
        using namespace qnject::api;


        // handlers that target a QObject by address
        const auto byAddressHandlers =
                prefix("by-address", any_of(
//                        prefix("click", qobject_at_address_handler(click_qwidget)),
                        prefix("grab", qobject_at_address_handler(qwidgets_grab_image)),
                        prefix("menu", qobject_at_address_handler(get_menu_tree)),
                        prefix("trigger-menu-action",
                               qobject_at_address_handler(menu_tree_trigger_action)),
                        get(qobject_at_address_json_handler(qwidget_show)),
                        post(qobject_at_address_handler(qwidget_update)),
                        handler(method_not_found)
                ));


        // Show the widgets list
        const auto widgetListHandler = get(handler(wrap_json(qwidget_list)));

        const auto qWidgetPath =
                prefix("api",
                       prefix("qwidgets", any_of(
                               leaf(widgetListHandler),
                               byAddressHandlers,

                               // Returns the main menu object(s) of the application
                               prefix("main-menu", handler(wrap_json(qwidgets_get_menubar))),

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
