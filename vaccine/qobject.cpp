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

extern "C" void qt_addObject(QObject *obj)
{
  printf("QObject added\n");

  // From GammaRay
  static void (*next_qt_addObject)(QObject *obj)
    = (void (*)(QObject *obj))dlsym(RTLD_NEXT, "qt_addObject");
  next_qt_addObject(obj);
}

#endif // HAVE QT5CORE && QT5WIDGES
