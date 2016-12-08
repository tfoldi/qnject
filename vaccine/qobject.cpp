#include "../config.h"
#if HAVE_QT5CORE && HAVE_QT5WIDGETS 
#include "vaccine.h"

#include <dlfcn.h>
#include <map>
#include <QApplication>
#include <QList>
#include <QObject>
#include <QWidget>
#include <QWindow>
#include <QBuffer>
#include <QByteArray>
#include <QMetaObject>
#include <QMetaProperty>

#include "../deps/json/json.hpp"

namespace vaccine {

  template<typename Functor>
    bool with_object(nlohmann::json & req, nlohmann::json & resp, Functor functor)
    {
      resp["error"] = "object not found";

      for( QWidget * child : qApp->allWidgets() ) {
        if (child && child->objectName() == req["object"].get<std::string>().c_str() ) {
          resp["error"] = "";

          functor(child);
          return true;
        }
      }
      return false;
    }

  // XXX: this should go to vaccine.cpp or .h? or request_utils?
  void parse_request_body(struct http_message *hm, nlohmann::json & req)
  {
    if ( hm->body.len > 0 ) {
      std::string s;
      s.assign(hm->body.p, hm->body.len);
      try {
        req = nlohmann::json::parse(s);
      } catch (std::exception & ex ) {
        printf("Request body: %s\n", ex.what());
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

    // get request data
    parse_request_body(hm,req);

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
          resp["widgets"].push_back( qPrintable(child->objectName()) );
        }
      }

      // Windows and their children - do I need them?
      for( QWindow * child : qApp->allWindows() ) {
        resp["windows"].push_back( qPrintable(child->objectName()) );

        for( QObject * obj : child->findChildren<QObject *>() )
          resp["windows"][qPrintable(child->objectName())].push_back( qPrintable(obj->objectName()) );
      }
    } else if (uri == "qobject/getProperties") {
      with_object(req, resp, [&](QObject * obj) {
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
      with_object(req, resp, [&](QObject * obj) {
          QVariant val = obj->property( req["name"].get<std::string>().c_str() );
          resp[ "name" ] = qPrintable(val.toString()) ;
          });
    } else if (uri == "qobject/setProperty") {
      with_object(req, resp, [&](QObject * obj) {
          obj->setProperty( req["name"].get<std::string>().c_str(), req["value"].get<std::string>().c_str());
          });
    } else if (uri == "qobject/grab") {
      // check if we found our widget
      if ( ! with_object(req, resp, [&](QWidget * obj) {
          QByteArray bytes;
          QBuffer buffer(&bytes);
          buffer.open(QIODevice::WriteOnly);
          obj->grab().save(&buffer, "PNG"); 

          mg_send_head(nc, 200, bytes.length(), "Content-Type: image/png");
          mg_send(nc, bytes.constData() ,bytes.length());
          })) 
      {
        // we didn't find widget like this
        mg_http_send_error(nc, 404, "Widget not found");
        return;
      }

    } else {
      mg_http_send_error(nc, 404, "Method not found");
      return;
    }

    send_json(nc,resp);
  }

  __attribute__((constructor))
    static void initializer(void) {
      printf("[%s] Register qobject service\n", __FILE__);
      vaccine::register_callback("qobject", qobject_handler);
    }

}

#endif // HAVE QT5CORE && QT5WIDGES
