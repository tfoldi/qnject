#pragma once

#include <QMenu>
#include <QMenuBar>
#include <QApplication>
#include "./base.h"
#include "../qwidget-json-helpers.h"
#include "../qobject-utils.h"
#include "menu-tree-get.h"

namespace qnject {
    namespace api {


        // Gets the actions for a menu
        nlohmann::json qwidgets_get_menubar(const Request& r) {
            return helpers::collectObjectsJson<QMenuBar>(qApp->allWidgets(), impl::dumpMenu);
        }


    }
}
