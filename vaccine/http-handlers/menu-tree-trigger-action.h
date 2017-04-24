#pragma once

#include "./base.h"
#include <QMenu>
#include <QMenuBar>

namespace qnject {
    namespace api {

        // FWD
        namespace impl { Json dumpMenu(QObject* o); }


        // triggers an action in the menu
        data_response_t menu_tree_trigger_action(const Request& r, QObject* obj) {
            return brilliant::response::error(500, "Not implemented");
        }
    }
}
