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
#include "qwidget.json.h"


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

  void get_all_qwidgets(nlohmann::json const & req, nlohmann::json & resp)
  {
    char buf[40];

    LOG_SCOPE_FUNCTION(INFO);

    sprintf(buf,"%p",qApp);

    resp["qApp"] = buf;
    resp["appName"] = qPrintable(QApplication::applicationName());

    // All widgets
    for( QWidget * child : qApp->allWidgets() ) {
      if (child && child->objectName() != "")  {
        resp["widgets"].push_back( 
            {
            {"objectName",  qPrintable(child->objectName())},
            {"parentName", child->parent() ? qPrintable(child->parent()->objectName()) : "" },
            {"obectKind", "widget" },
            {"className", child->metaObject()->className() },
            {"superClass", child->metaObject()->superClass()->className() }
            }
            );
      }
    }
  }

  void qwidget_handler( 
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
    if ( uri == "qwidgets" && is_request_method(hm,"GET") ) {
      get_all_qwidgets(req,resp);
    } else if ( splitURI.size() == 2 && is_request_method(hm,"GET") ) {
      DLOG_F(INFO, "Requesting data from object %s", objectName);
      with_object(objectName, statusCode, [&](QObject * obj) {
          const QMetaObject* metaObject = obj->metaObject();

          for(auto i = 0; i < metaObject->propertyCount(); ++i) {
          const char * propertyName = metaObject->property(i).name();
          QVariant val = metaObject->property(i).read( obj );
          resp[ "properties" ].push_back( { propertyName, qPrintable(val.toString()), "meta" } );
          }

          for (auto qbaPropertyName : obj->dynamicPropertyNames() ) {
          auto propertyName = qbaPropertyName.constData();
          QVariant val = obj->property( propertyName );
          resp[ "properties" ].push_back( { propertyName, qPrintable(val.toString()), "dynamic" } );
          }
          });
    } else if (uri == "qobject/getProperty") {
      with_object(objectName, statusCode, [&](QObject * obj) {
          QVariant val = obj->property( req["name"].get<std::string>().c_str() );
          resp[ "name" ] = qPrintable(val.toString()) ;
          });
    } else if (uri == "qobject/setProperty") {
      with_object(objectName, statusCode, [&](QObject * obj) {
          obj->setProperty( req["name"].get<std::string>().c_str(), req["value"].get<std::string>().c_str());
          });
    } else if (uri == "qobject/grab") {
      // check if we found our widget
      with_object(objectName, statusCode, [&](QWidget * obj) {
          QByteArray bytes;
          QBuffer buffer(&bytes);
          buffer.open(QIODevice::WriteOnly);
          obj->grab().save(&buffer, "PNG"); 

          mg_send_head(nc, 200, bytes.length(), "Content-Type: image/png");
          mg_send(nc, bytes.constData() ,bytes.length());
          });

      if ( statusCode == 200 ) {
        // we already sent back the reply, nothing to do
        return;
      } else {
        // we didn't find widget like this
        resp["error"] = "Widget not found";
      }
    } else {
      statusCode = 404;
      resp["error"] = "Method not found";
    }

    send_json(nc,resp,statusCode);
  }

  __attribute__((constructor))
    static void initializer(void) {
      DLOG_F(INFO, "Register qwidget service");
      vaccine::register_callback("qwidgets", qwidget_handler, qwidget_json);
    }

}

#endif // HAVE QT5CORE && QT5WIDGES
