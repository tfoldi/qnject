#pragma once

#include <QMenu>
#include <QMenuBar>
#include "./base.h"
#include "../qwidget-json-helpers.h"
#include "../qobject-utils.h"

namespace qnject {
    namespace api {

        // FWD
        namespace impl { Json dumpMenu(QObject* o); }

        // Gets the actions for a menu
        data_response_t get_menu_tree(const Request& r, QObject* obj) {
            return json_response(200, impl::dumpMenu(obj));
        }


        namespace impl {

            // Exports QMenu data as JSON
            Json dumpMenu(QObject* o) {
                using namespace brilliant;

                return {{"actions",    helpers::object_actions_meta(o)},
                        {"childMenus", helpers::collectChildrenJson<QMenu>(o, dumpMenu)},
                        {"object",     helpers::object_meta(o)}};

            }

        }
    }
}