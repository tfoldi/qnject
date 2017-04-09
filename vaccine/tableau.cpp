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
#include <QMouseEvent>


#include "../deps/loguru/loguru.hpp"
#include "../deps/json/json.hpp"

#include "utils.hpp"
#include "vaccine.h"


nlohmann::json get_all_superclass(QObject * obj) {
  nlohmann::json ret;

  for ( auto super = obj->metaObject()->superClass(); strcmp(super->className(),"QObject") != 0; super = super->superClass() )
    ret.push_back( super->className() );

  return ret;
}

namespace vaccine {

  static void (*toggle_check)(QObject*,int, QList<QModelIndex> const&) = nullptr;
  static void (*set_radio_mode)(QObject*,bool) = nullptr;

  void set_tableau_hook_ptrs(void) {
    toggle_check = (void(*)(QObject*,int, QList<QModelIndex> const&)) dlsym( RTLD_DEFAULT, "_ZN14CheckListModel11ToggleCheckEiRK5QListI11QModelIndexE");
    set_radio_mode = (void(*)(QObject*,bool)) dlsym( RTLD_DEFAULT, "_ZN14CheckListModel12SetRadioModeEb");
  }

  template <typename Fn>
  void with_object(const char* objectName, Fn fn) {
      for( QWidget * child : qApp->allWidgets() ) {
        if (child && child->objectName() == objectName ) {
          fn(child);
        }
      }
  }

  template<typename Functor>
  void with_object(const char * objectName, int & statusCode, Functor functor)
  {
    statusCode = 404;
    with_object( objectName, [&](QObject* o){
        statusCode = 200;
        functor(o);
    });
  }


  // Apply the function on the children of an object that have the specified class name
  template <typename T, typename Fn>
  void with_children(QObject* o, const char* className, Fn fn) {
    for( T* child : o->findChildren<T*>() ) {
      // check for failiure signs
      if (strcmp(child->metaObject()->className(),className) == 0) {
        fn(child);
      }
    }
  }

  template <typename T, typename Fn>
  void with_children_of_object(const char* objectName, const char* className, Fn fn) {
    with_object(objectName, [&](QObject* o){
        with_children<T>(o, className, fn);
    });
  }

  template <typename T, typename Fn>
  void respond_with_children_of_object(const char* objectName, const char* className, int& statusCode, Fn fn) {
    statusCode = 404;
    with_children_of_object<T>( objectName, className, [&](T* o){
        statusCode = 200;
        fn(o);
    });
  }




  enum RequestMethod {
    kREQUEST_GET,
    // for CORS preflight requests
    kREQUEST_OPTIONS,
    kREQUEST_POST,
  };

  // Checks if the request is a message
  RequestMethod request_method(struct http_message* hm) {
    static const struct mg_str GET =  mg_mk_str("GET");
    static const struct mg_str POST =  mg_mk_str("POST");
    if (mg_strcmp(hm->method, GET) == 0) return kREQUEST_GET;
    if (mg_strcmp(hm->method, POST) == 0) return kREQUEST_POST;
    else return kREQUEST_OPTIONS;
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


    // set function pointers to call Tableau functions directly
    if ( toggle_check == nullptr || set_radio_mode == nullptr )
      set_tableau_hook_ptrs();

    // Distpatch URI handlers in a big fat branch
    // test URI: QuickFilterCategoricalWidgetSample - Superstorenone:Segment:nk
    if (splitURI[0] == "tableau" && splitURI[1] == "filter" and objectName != "" ) {

      int rowsIdx = 0;
      respond_with_children_of_object<QAbstractItemModel>(objectName.c_str(), "CheckListModel", statusCode, [&](QAbstractItemModel* child){

              resp["children"].push_back( 
                  { {"objectName",  qPrintable(child->objectName())},
                  {"parentName", child->parent() ? qPrintable(child->parent()->objectName()) : "" },
                  {"className", child->metaObject()->className() },
                  {"superClass", get_all_superclass(child)} }
                  );



              // no children? no list
              if (child->rowCount() == 0) { return; };

              // key the model list by its memory address (LOL)
              std::string modelKey = std::to_string((uintptr_t)child);

              /* DLOG_F(INFO, "CLM ! %s rows: %d, c %d", child->metaObject()->className(), child->rowCount(), */
              /*     child->columnCount()); */ 

              for ( auto i = 0; i < child->rowCount() ; i++ ) { 
                  try {
                    QModelIndex items = child->index(i,0); 

                    // are these thigs ok?
                    if (!items.isValid() ) {
                      DLOG_F(ERROR, "row: %d, col: %d, type: %s", items.row(), items.column(), "invalid"); 
                      continue;
                    }

                    // Assume that we only care about the first list
                      // Toggle the checkbox
                      if ( req["body"].is_object() && req["body"]["toggle"].is_object() && req["body"]["toggle"]["row"].get<int>() == items.row() ) {
                        if (modelKey == req["body"]["toggle"]["modelPtr"].get<std::string>()) {
                          DLOG_F(INFO, "Toggle checkbox on row: %d to %d", items.row(), req["body"]["toggle"]["value"].get<int>());
                          
                          
                          /*with_object(objectName.c_str(), statusCode, [&](QObject * obj) {

                          });*/
                          toggle_check(child, req["body"]["toggle"]["value"].get<int>(), QList<QModelIndex>({items}));
                        }
                      }


                    auto data = child->data(items, Qt::CheckStateRole); 
                    resp["rows"][modelKey][items.row()] = qPrintable(data.toString());

                    /* DLOG_F(INFO, "row: %d, col: %d, data: %s", items.row(), items.column(), qPrintable( data.toString() ) ); */ 
                  } catch (...) {
                    DLOG_F(ERROR, "Exception");
                    statusCode = 500;
                  }

              }

              // only care about the ones with the
              rowsIdx++;
        //   });
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
