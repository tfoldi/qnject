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
#include "dyld-hooking.h"

// REQUEST / RESPONSE ----------------------------------------------------------

using brilliant::Request;
using qnject::address_to_string;
using qnject::helpers::object_meta;
//
// Method / Signal / Slot metadata --------------------------------------------




// -- HANDLERS



namespace {







        using namespace brilliant::route;
        using namespace qnject::api;


        template<typename Resolver>
        decltype(auto) byAddressHandlers(std::string pathPrefix, Resolver resolver) {
            return prefix(pathPrefix, any_of(
                    prefix("grab", resolver(qwidgets_grab_image)),
                    prefix("menu", resolver(get_menu_tree)),
                    prefix("trigger", resolver(menu_tree_trigger_action)),
                    get(resolver(json_wrapped_result(qwidget_show))),
                    post(resolver(qwidget_update)),
                    handler(method_not_found)
            ));

        }

}

namespace serums {
	namespace qwidgets {
		// Show the widgets list
		const auto widgetListHandler = get(handler(wrap_json(qwidget_list)));

		const auto qWidgetPath =
			prefix("api",
				prefix("qwidgets", any_of(
					leaf(widgetListHandler),

					// Do things to widgets only. Should be safe with any input
					byAddressHandlers("by-address", [](auto&& fn) { return qobject_at_address_handler(fn); }),

					// Segfault on bad input, but access everything by address
					byAddressHandlers("by-address-unsafe", [](auto&& fn) { return qobject_at_address_handler_unsafe(fn); }),

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

		using qnject::api::method_not_found;
		using brilliant::request::fromBody;



        using namespace brilliant::route;

        auto r = fromBody(nc, hm);

        DLOG_F(INFO, "Serving URI: \"%s %s\" with post data >>>%.*s<<<",
               r.req["method"].get<std::string>().c_str(),
               uri.c_str(),
               (int) hm->body.len,
               hm->body.p);


		auto handled = serums::qwidgets::qWidgetPath(make(r));

        // if we get here then we arent in the qwidget path.
        if (!handled) {
            method_not_found(r);
        }

    }


}



//namespace {
//    //__attribute__((constructor))
//    void dyld_initialize(void) {
//        DLOG_F(INFO, "Register qwidget service");
//        vaccine::register_callback("qwidgets", vaccine::qwidget_handler, qwidget_json);
//    }
//
//    //__attribute__((constructor))
//    void dyld_finalize(void) {
//        DLOG_F(INFO, "Unregistering qwidget service");
//    }
//
//}
//

//#define REGISTER_VACCINE_HANDLER(prefix, handler) \
//	namespace prefix { \
//		const auto handlerFn = handler;\
//	}
//
//REGISTER_VACCINE_HANDLER(qwidgets, api::qWidgetPath)
//

#endif // HAVE QT5CORE && QT5WIDGES
