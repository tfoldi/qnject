#pragma once

#include <QMenu.h>
#include <QMenuBar.h>
#include "./base.h"
#include "../qwidget-json-helpers.h"

namespace qnject {
    namespace api {

        // FWD
        namespace impl { Json dumpMenu(QObject* o); }


        // Gets the actions for a menu
        data_response_t get_menu_tree(const Request& r, QObject* obj) {
            using namespace brilliant;
            Json j = "{ \"actions\":[], \"children\":[] }"_json;

            if (qobject_cast<QMenuBar*>(obj) == nullptr) {
                return response::error(403, "Cannot cast target object to QMenuBar");
            }


            for (QObject* o : obj->children()) {
                j["children"].push_back(impl::dumpMenu(o));
            }

            return json_response(200, j);
        }


        namespace impl {


            // Exports QMenu data as JSON
            Json dumpMenu(QObject* o) {
                using namespace brilliant;
                using qnject::helpers::object_meta;

                // Try to cast to QMenu
                QMenu* m = qobject_cast<QMenu*>(o);
                if (m == nullptr) { return {{"error", "Cannot cast object to QMenu*"}}; };

                // Build output data
                Json j = {{"actions",  {}},
                          {"children", {}},
                          {"meta",     object_meta(o)}};

                // Add actions
                for (QAction* action: m->actions()) {
                    j["actions"].push_back(qnject::helpers::action_meta(action));
                }

                // Add children
                for (QObject* o : m->children()) {
                    j["children"].push_back(dumpMenu(o));
                }

                return j;
            }


        }
    }
}