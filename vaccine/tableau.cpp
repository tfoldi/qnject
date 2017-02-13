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


static void (*toggle_check)(QObject*,int, QList<QModelIndex> const&) = (void(*)(QObject*,int, QList<QModelIndex> const&)) dlsym( RTLD_DEFAULT, "_ZN14CheckListModel12SetRadioModeEb");
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
    if (uri == "tableau/filter" ) {
      with_object("QuickFilterCategoricalWidgetSample - Superstorenone:Segment:nk", statusCode, [&](QObject * obj) {
        for( QAbstractItemModel * child : obj->findChildren<QAbstractItemModel *>() ) {
          resp["children"].push_back( 
                                     { {"objectName",  qPrintable(child->objectName())},
                                       {"parentName", child->parent() ? qPrintable(child->parent()->objectName()) : "" },
                                       {"className", child->metaObject()->className() },
                                       {"superClass", get_all_superclass(child)} }
                                    );

          if (! strcmp(child->metaObject()->className(),"CheckListModel") ) {
            QAbstractItemModel * atm = child; 

            DLOG_F(INFO, "CLM ! %s rows: %d, c %d", child->metaObject()->className(), atm->rowCount(),
                   atm->columnCount()); 
            for ( auto i = 0; i < atm->rowCount() ; i++ ) { 
              try {
                QModelIndex items = atm->index(i,0); 

                if ( i == 2 && atm->rowCount() == 4) {
                  QList<QModelIndex> checked;
                  checked.append(items);
                  DLOG_F(INFO, "Break me here");
                }


                if (items.isValid() ) {
                  auto data = atm->data(items, Qt::CheckStateRole); 
                  DLOG_F(INFO, "row: %d, col: %d, data: %s", items.row(), items.column(), qPrintable( data.toString() ) ); 
                } else {
                  DLOG_F(ERROR, "row: %d, col: %d, type: %s", items.row(), items.column(), "invalid"); 
                }
              } catch (...) {
                DLOG_F(ERROR, "Exception");
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
      vaccine::register_callback("filter", tableau_handler, NULL);
    }

}

#endif // HAVE QT5CORE && QT5WIDGES
