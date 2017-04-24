#pragma once

#include <QObject>

namespace qnject {
    namespace helpers {


            template <typename ChildT, typename Fn, typename T>
            decltype(auto) foldChildrenCastableTo(Fn fn, T init, QObject* m) {
                for (QObject* o : m->children()) {
                    ChildT* child = qobject_cast<ChildT*>(o);
                    // skip non-menu children
                    if (child == nullptr) continue;

                    // call the folder function
                    fn(child, init);

                }
                return init;
            }

            template <typename ChildT, typename Fn>
            nlohmann::json collectChildrenCastableTo(QObject* m, Fn fn) {
                return foldChildrenCastableTo<ChildT>(
                    [&](ChildT* c, nlohmann::json& j) { return j.push_back(fn(c)); },
                    "[]"_json,
                    m );
            }
    }
}