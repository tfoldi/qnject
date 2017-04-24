#pragma once

#include <QObject>
#include <QDebug>

namespace qnject {
    namespace helpers {


        template<typename ChildT, typename Fn, typename T, typename QObjects>
        decltype(auto) foldChildrenCastableBase(Fn fn, T init, QObjects objs) {
            for (QObject* o : objs) {
                ChildT* child = qobject_cast<ChildT*>(o);
                if (child == nullptr) continue;
                fn(child, init);
            }
            return init;
        }


        template<typename ChildT, typename Fn, typename T>
        decltype(auto) foldChildrenCastableTo(Fn fn, T init, QObject* m) {
            return foldChildrenCastableBase<ChildT>(fn, init, m->children());
        }

        template<typename ChildT, typename QObjects, typename Fn>
        nlohmann::json collectObjectsJson(QObjects&& objs, Fn fn) {
            return foldChildrenCastableBase<ChildT>(
                    [&](ChildT* c, nlohmann::json& j) { return j.push_back(fn(c)); },
                    "[]"_json,
                    objs);
        }

        template<typename ChildT, typename Fn>
        nlohmann::json collectChildrenJson(QObject* m, Fn fn) {
            return collectObjectsJson<ChildT>(m->children(), fn);
        }


    }
}