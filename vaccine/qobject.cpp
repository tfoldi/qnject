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

#include "../deps/json/json.hpp"

namespace vaccine {

#ifdef WITH_QT_STALKER
  static std::map<std::string,void *> s_qobject_map;
#endif

  void qobject_handler( 
      std::string & uri, 
      struct mg_connection *nc,  
      void *ev_data,            
      struct http_message *hm)
  {
    nlohmann::json resp;
    char buf[140];

    sprintf(buf,"qApp:%p",qApp);

    resp["qApp"] = buf;
    resp["appName"] = qPrintable(QApplication::applicationName());
    
    for( QObject * child : qApp->findChildren<QObject *>() )
      resp["child"].push_back( qPrintable(child->objectName()) );

    for( QObject * child : qApp->allWidgets() ) {
      if (child && child->objectName() != "")  {
        resp["widgets"].push_back( qPrintable(child->objectName()) );
        if ( child->objectName() == "m_edtExplicitPassword" ) {
          child->setProperty("text", QVariant("1234"));
        }
      }
    }

    for( QWindow * child : qApp->allWindows() ) {
      resp["windows"].push_back( qPrintable(child->objectName()) );

      for( QObject * obj : child->findChildren<QObject *>() )
        resp["windows"][qPrintable(child->objectName())].push_back( qPrintable(obj->objectName()) );
    }


    send_json(nc,resp);
  }

  __attribute__((constructor))
    static void initializer(void) {
        printf("[%s] Register qobject service\n", __FILE__);
      vaccine::register_callback("qobject", qobject_handler);
    }

}

#ifdef WITH_QT_STALKER
void QObject::setObjectName(const QString & name ) {
  typedef void (QObject::*f_setObjectName)(const QString & name);
  static f_setObjectName origMethod = 0;

  printf("Register object: %p %s\n", &name, qPrintable(name));

  if (origMethod == 0)
  {
    void *tmpPtr = dlsym(RTLD_NEXT, "_ZN7QObject13setObjectNameERK7QString");
 
    if (tmpPtr)
      memcpy(&origMethod, &tmpPtr, sizeof(void *));
    else
      printf("BAJ VAN\n");
  }
 
  // here we call the original method
  if (origMethod)
    (this->*origMethod)(name);
}

QObject::~QObject() {
  typedef void (QObject::*f_destructor)();
  static f_destructor origMethod = 0;

  printf("elment: %s (%p)\n", qPrintable(this->objectName()), this);

  if (origMethod == 0)
  {
    void *tmpPtr = dlsym(RTLD_NEXT, "_ZN7QObjectD0Ev");
 
    if (tmpPtr)
      memcpy(&origMethod, &tmpPtr, sizeof(void *));
    else
      printf("BAJ VAN\n");
  }
 
  // here we call the original method
  if (origMethod)
    (this->*origMethod)();
}
#endif // WITH_QT_STALKER

#endif // HAVE QT5CORE && QT5WIDGES
