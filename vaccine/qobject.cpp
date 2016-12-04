#include "../config.h"
#if HAVE_QT5CORE && HAVE_QT5WIDGETS 
#include "vaccine.h"
#include <QApplication>

namespace vaccine {

  void qobject_handler( 
      std::string & uri, 
      struct mg_connection *nc,  
      void *ev_data,            
      struct http_message *hm)
  {
    mg_send_head(nc, 200, 21, "Content-Type: text/plain");
    mg_printf(nc,"qApp:%p",qApp);
  }

  __attribute__((constructor))
    static void initializer(void) {
      printf("[%s] Register qobject service\n", __FILE__);
      vaccine::register_callback("qobject", qobject_handler);
    }

}

#endif // HAVE QT5CORE && QT5WIDGES
