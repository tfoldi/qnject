#include "qnject_config.h"
#if HAVE_QT5CORE && HAVE_QT5WIDGETS 

#include <dlfcn.h>
#include <map>
#include <vector>
#include <QApplication>
#include <QList>
#include <QObject>
#include <QWidget>
#include <QBuffer>
#include <QByteArray>
#include <QMetaObject>
#include <QMetaProperty>

#include "../deps/loguru/loguru.hpp"
#include "../deps/json/json.hpp"

#include "utils.hpp"
#include "vaccine.h"


QObject * (*filter_model)(QObject*) = (QObject * (*)(QObject*)) dlsym( RTLD_DEFAULT, "_ZNK32QuickFilterCategoricalWidget_Old14GetFilterModelEv");
//QObject * (*filter_model)(QObject*) = (QObject * (*)(QObject*)) dlsym( RTLD_DEFAULT, "_ZNK32QuickFilterCategoricalWidget_Old8GetModelEv");


namespace vaccine {

  template<typename Functor>
    void with_object(const char * objectName, int & statusCode, Functor functor)
    {
      statusCode = 404;

      for( QWidget * child : qApp->allWidgets() ) {
        if (child && child->objectName() == objectName ) {
          statusCode = 200;

          functor(child);
        }
      }
    }

  void tableau_handler( 
      std::string & uri, 
      struct mg_connection *nc,  
      void *ev_data,            
      struct http_message *hm)
  {
    nlohmann::json resp, req;
    int statusCode = 200;
    const char * objectName = "";
    std::vector<std::string> splitURI = split(uri.c_str(),'/');

    // get request data
    parse_request_body(hm,req);
    if (splitURI.size() > 1)
      objectName = splitURI[1].c_str();

    DLOG_F(INFO, "Serving URI: \"%s %s\" with post data >>>%.*s<<<", 
        req["method"].get<std::string>().c_str(),
        uri.c_str(),
        (int)hm->body.len,
        hm->body.p);


    // Distpatch URI handlers in a big fat branch
    // 
    if (uri == "tableau" ) {
      with_object("QuickFilterCategoricalWidgetSample - Superstorenone:Segment:nk", statusCode, [&](QObject * obj) {
          if (filter_model) {
          DLOG_F(INFO, "calling filter model. obj=%p, filter_model=%p", obj, filter_model);
          QObject * ret = filter_model(obj);

          DLOG_F(INFO, "filter model done, ret=%p", ret);
 //         DLOG_F(INFO, "metaObject==%p", ret->metaObject());

//          resp["superClass"] = ret->metaObject()->superClass()->className();
//          resp["retobj"] = ret->metaObject()->className();
          } else {
          statusCode = 404;
          }
          });
    } else {
      statusCode = 404;
      resp["error"] = "Method not found";
    }

    send_json(nc,resp,statusCode);
  }

  __attribute__((constructor))
    static void initializer(void) {
      DLOG_F(INFO, "Register tableau service");
      vaccine::register_callback("tableau", tableau_handler, NULL);
    }

}

#endif // HAVE QT5CORE && QT5WIDGES
