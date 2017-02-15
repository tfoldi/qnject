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
#include <QItemSelectionModel>
#include <QAbstractTableModel>
#include <QListWidget>
#include <QListView>
#include <QException>
#include <QMap>
#include <QVariant>
#include <QDebug>


#include "../deps/loguru/loguru.hpp"
#include "../deps/json/json.hpp"

#include "utils.hpp"
#include "vaccine.h"


static void (*toggle_check)(QObject*,int, QList<QModelIndex> const&) = (void(*)(QObject*,int, QList<QModelIndex> const&)) dlsym( RTLD_DEFAULT, "_ZN14CheckListModel11ToggleCheckEiRK5QListI11QModelIndexE");
static void (*set_radio_mode)(QObject*,bool) = (void(*)(QObject*,bool)) dlsym( RTLD_DEFAULT, "_ZN14CheckListModel12SetRadioModeEb");

nlohmann::json get_all_superclass(QObject * obj) {
  nlohmann::json ret;

  for ( auto super = obj->metaObject()->superClass(); strcmp(super->className(),"QObject") != 0; super = super->superClass() )
    ret.push_back( super->className() );

  return ret;
}

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


  enum RequestMethod {
    kREQUEST_GET,
    kREQUEST_POST,
  };

  // Checks if the request is a message
  RequestMethod request_method(struct http_message* hm) {
    static const struct mg_str GET =  mg_mk_str("GET");
    if (mg_strcmp(hm->method, GET) == 0) return kREQUEST_GET;
    else return kREQUEST_POST;
  }


  void tableau_handler( 
                       std::string & uri, 
                       struct mg_connection *nc,  
                       void *ev_data,            
                       struct http_message *hm)
  {
    nlohmann::json resp, req;
    int statusCode = 200;
    std::string objectName = "";
    std::vector<std::string> splitURI = split(uri.c_str(),'/');
    RequestMethod method = request_method(hm);

    // get request data
    parse_request_body(hm,req);
    if (splitURI.size() > 2)
      objectName = urldecode2(splitURI[2]);

    DLOG_F(INFO, "Serving URI: \"%s %s\" with post data >>>%.*s<<< on object %s", 
           req["method"].get<std::string>().c_str(),
           uri.c_str(),
           (int)hm->body.len,
           hm->body.p,
           objectName.c_str());


    // Distpatch URI handlers in a big fat branch
    // test URI: QuickFilterCategoricalWidgetSample - Superstorenone:Segment:nk
    if (splitURI[0] == "tableau" && splitURI[1] == "filter" and objectName != "" ) {
      with_object(objectName.c_str(), statusCode, [&](QObject * obj) {
        for( QAbstractItemModel * child : obj->findChildren<QAbstractItemModel *>() ) {
          resp["children"].push_back( 
                                     { {"objectName",  qPrintable(child->objectName())},
                                       {"parentName", child->parent() ? qPrintable(child->parent()->objectName()) : "" },
                                       {"className", child->metaObject()->className() },
                                       {"superClass", get_all_superclass(child)} }
                                    );

          // if we got a get request then there is no data for us
          if (method == kREQUEST_GET) {
            statusCode = 200;
            return;
          }

          if (! strcmp(child->metaObject()->className(),"CheckListModel") && child->rowCount() == 4 /* XXX */ ) {
            QAbstractItemModel * atm = child; 

            DLOG_F(INFO, "CLM ! %s rows: %d, c %d", child->metaObject()->className(), atm->rowCount(),
                   atm->columnCount()); 
            for ( auto i = 0; i < atm->rowCount() ; i++ ) { 
              try {
                QModelIndex items = atm->index(i,0); 

                if (items.isValid() ) {
                  if ( req["body"].is_object() && req["body"]["toggle"].is_object() && req["body"]["toggle"]["row"].get<int>() == items.row() ) {
                    DLOG_F(INFO, "Toggle checkbox on row: %d to %d", items.row(), req["body"]["toggle"]["value"].get<int>());
                    toggle_check(atm, req["body"]["toggle"]["value"].get<int>(), QList<QModelIndex>({items}));
                  }

                  auto data = atm->data(items, Qt::CheckStateRole); 
                  resp["rows"][items.row()] = qPrintable(data.toString());
                  DLOG_F(INFO, "row: %d, col: %d, data: %s", items.row(), items.column(), qPrintable( data.toString() ) ); 
                } else {
                  DLOG_F(ERROR, "row: %d, col: %d, type: %s", items.row(), items.column(), "invalid"); 
                }
              } catch (...) {
                DLOG_F(ERROR, "Exception");
                statusCode = 500;
              }

            }

          } else {
            statusCode = 404;
          }
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
