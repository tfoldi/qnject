#pragma once

#include "loguru.hpp"
#include "./base.h"
#include <QMenu>
#include <QAction>
#include <QMenuBar>

namespace qnject {
    namespace api {

        // FWD
        namespace impl { Json dumpMenu(QObject* o); }


        // triggers an action in the menu
        data_response_t menu_tree_trigger_action(const Request& r, QObject* obj) {
            LOG_SCOPE_FUNCTION(INFO);

            QAction* action = qobject_cast<QAction*>(obj);
            if (action == nullptr) { return brilliant::response::error(404, "Selected QObject is not a QAction"); }

            DLOG_F(INFO, "Triggering action '%s' @ %s", action->text().toStdString().c_str(), address_to_string(action).c_str());
            action->trigger();
            return json_response(200, {"ok"});
        }
    }
}
