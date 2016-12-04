#include "../config.h"
#if HAVE_QT5CORE && HAVE_QT5WIDGETS 
#include "vaccine.h"

#include <dlfcn.h>
#include <QApplication>
#include <QList>
#include <QObject>
#include <QWidget>

#include "../deps/json/json.hpp"

namespace vaccine {

  void qobject_handler( 
      std::string & uri, 
      struct mg_connection *nc,  
      void *ev_data,            
      struct http_message *hm)
  {
    nlohmann::json j;
    char buf[140];

    sprintf(buf,"qApp:%p",qApp);

    j["qApp"] = buf;
    j["appName"] = qPrintable(QApplication::applicationName());

    send_json(nc,j);
  }

  __attribute__((constructor))
    static void initializer(void) {
        printf("[%s] Register qobject service\n", __FILE__);
      vaccine::register_callback("qobject", qobject_handler);
    }

}

void QObject::setObjectName(const QString & name ) {
  typedef void (*f_setObjectName)(void * dis, const QString & name);
  static f_setObjectName origMethod = 0;

  printf("anyad: %s\n", qPrintable(name));

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
    (*origMethod)(this,name);
}

#endif // HAVE QT5CORE && QT5WIDGES
