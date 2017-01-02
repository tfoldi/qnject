#include "../config.h"
#if HAVE_QT5CORE && HAVE_QT5WIDGETS 

#include <dlfcn.h>
#include <map>
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

#include "vaccine.h"
#include "qobject.json.h"

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

  void qobject_handler( 
      std::string & uri, 
      struct mg_connection *nc,  
      void *ev_data,            
      struct http_message *hm)
  {
    nlohmann::json resp, req;
    int statusCode = 200;
    const char * objectName = "";

    // get request data
    parse_request_body(hm,req);
    if ( req["object"] != nullptr && ! req["object"].empty() )
      objectName = req["object"].get<std::string>().c_str();

    // URI handlers in a big fat branch
    // 
    if ( uri == "qobject" || uri == "qobject/list" ) {
      char buf[40];
      sprintf(buf,"qApp:%p",qApp);

      resp["uri"] = uri;
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

    } else if (uri == "qobject/getProperties") {
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
      resp["error"] = "Method not found";
    }

    send_json(nc,resp,statusCode);
  }

  __attribute__((constructor))
    static void initializer(void) {
      DLOG_F(INFO, "Register qobject service");
      vaccine::register_callback("qobject", qobject_handler, qobject_json);
    }

}

#endif // HAVE QT5CORE && QT5WIDGES
