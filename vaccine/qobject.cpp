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

#include "../deps/json/json.hpp"

namespace vaccine {

  void qobject_handler( 
      std::string & uri, 
      struct mg_connection *nc,  
      void *ev_data,            
      struct http_message *hm)
  {
    nlohmann::json resp, req;

    // get request data
    if ( hm->body.len > 0 ) {
      std::string s;
      s.assign(hm->body.p, hm->body.len);
      try {
        req = nlohmann::json::parse(s);
      } catch (std::exception & ex ) {
        printf("Request body: %s\n", ex.what());
      }
    }

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

      // Windows and their children
      for( QWindow * child : qApp->allWindows() ) {
        resp["windows"].push_back( qPrintable(child->objectName()) );

        for( QObject * obj : child->findChildren<QObject *>() )
          resp["windows"][qPrintable(child->objectName())].push_back( qPrintable(obj->objectName()) );
      }
    } else if (uri == "qobject/setProperty") {
      for( QWidget * child : qApp->allWidgets() ) {
        if (child && child->objectName() == req["object"].get<std::string>().c_str() ) {
          child->setProperty( req["name"].get<std::string>().c_str(), req["value"].get<std::string>().c_str());
          resp[ "setProperty" ] = "success";
          break;
        }
      }
    } else if (uri == "qobject/grab") {
      for( QWidget * child : qApp->allWidgets() ) {
        if (child && child->objectName() == "mainFrame" ) {
          QByteArray bytes;
          QBuffer buffer(&bytes);
          buffer.open(QIODevice::WriteOnly);
          child->grab().save(&buffer, "PNG"); 

          mg_send_head(nc, 200, bytes.length(), "Content-Type: image/png");
          mg_send(nc, bytes.constData() ,bytes.length());          

          return;
        }
      }
    } else {
      mg_http_send_error(nc, 404, "Method not found");
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
