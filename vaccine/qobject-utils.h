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


    namespace helpers {

        void* stringToAddress(const std::string& addrStr) {
            if (addrStr[0] != '0' || addrStr[1] != 'x' || addrStr.length() > (sizeof(uintptr_t) * 2 + 2)) {
                DLOG_F(WARNING, "Invalid address given: %s" , addrStr.c_str());
                return nullptr;
            }

            // we should be safe here  (?)
            uintptr_t addr = std::stoull(addrStr, nullptr, 16);
            DLOG_F(INFO, "Decoded input address: %s -> %0llx" , addrStr.substr(2).c_str(), addr );
            return (void*)addr;

        }

    }
}