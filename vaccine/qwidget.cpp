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
#include <QAction>
#include <QMenu>
#include <QAbstractButton>

#include "../deps/loguru/loguru.hpp"
#include "../deps/json/json.hpp"

#include "utils.hpp"
#include "vaccine.h"
#include "qwidget.json.h"





// Misc QT helpers ----------------- --------------------------------------------

namespace {

  // Helper to invoke a function on a qobject if it has the specified name
  template<typename Functor>
  inline void with_object(const char * objectName, int & statusCode, Functor functor)
  {
    statusCode = 404;

    for( QWidget * child : qApp->allWidgets() ) {
      if (child && (child->objectName() == objectName ||
            objectName == std::to_string( (uintptr_t) child)) )
      {
        statusCode = 200;

        functor(child);
      }
    }
  }




  // Converts a pointer to a string
  std::string address_to_string(void* ptr) {
      return std::to_string( (uintptr_t) ptr);
  }

}

// Method / Signal / Slot metadata --------------------------------------------

namespace {
  using std::string;
  using std::vector;



  struct QObjectArg {
    string type;
    string name;
  };


  struct QObjectMethod {
    // PUBLIC, PRIVATE, PROTECTED
    string accessKind;

    // SIGNAL, SLOT, METHOD, COSTRUCTOR
    string type;

    string name;

    // The signature of the method
    string signature;

  };


  // Converts a QObject method type to its string repr
  string method_type_to_string(QMetaMethod::MethodType t) {
    switch (t) {
      case QMetaMethod::Signal: return "signal";
      case QMetaMethod::Slot: return "slot";
      case QMetaMethod::Method: return "method";
      case QMetaMethod::Constructor: return "constructor";
      default: return "<UNKNONW METHOD TYPE>";
    }
  }

  // Converts a QObject method type to its string repr
  string access_kind_to_string(QMetaMethod::Access t) {
    switch (t) {
      case QMetaMethod::Public: return "public";
      case QMetaMethod::Private: return "private";
      case QMetaMethod::Protected: return "protected";
      default: return "<UNKNONW METHOD ACCESS>";
    }
  }

  string qbyte_array_to_std_string(const QByteArray& bs) {
    return QString::fromLatin1(bs).toStdString();
  }



  // vector of string.... efficiency can suck it for now
  vector<QObjectMethod> get_qobject_method_metadata(const QMetaObject* metaObject) {

    vector<QObjectMethod> methods;



    for(int i = metaObject->methodOffset(); i < metaObject->methodCount(); ++i) {
      // get the meta object
      const auto& meth = metaObject->method(i);
      // append the data
      methods.emplace_back( QObjectMethod {
          access_kind_to_string(meth.access()),
          method_type_to_string(meth.methodType()),
          qbyte_array_to_std_string( meth.name() ),
          qbyte_array_to_std_string( meth.methodSignature() ),
      });
    }

    return methods;
  }


  void add_qobject_method_to_response(nlohmann::json& resp, const QObjectMethod& meth) {
      nlohmann::json local;
      local["name"] = meth.name;
      local["type"] = meth.type;
      local["access"] = meth.accessKind;
      local["signature"] = meth.signature;
      resp["methods"].push_back(local);
  }


}



// -- HANDLERS


namespace {


  // predicate for returning
  bool isScriptCommand(const QAction* action) {
    return action->text() == "Script Command";
  }

  template <typename triggerIf >
  void trigger_script_command(QAction * action, triggerIf pred) {
      // XXX: move to tableau.cpp or PATCH maybe?
      if (pred(action))
          action->activate( QAction::Trigger );
  }


  void add_property_to_response( nlohmann::json& resp, const char* kind, const char* propertyName, const QVariant val ) {
      resp[ "properties" ].push_back({
          {"name", propertyName},
          {"value", qPrintable(val.toString())},
          {"type", kind }
      });
  }

  // Helper for enumerating a QObjects fields
  void list_qobject_data_for_object_named(const char* objectName, struct mg_connection* nc)
  {
      int statusCode = 200;
      nlohmann::json resp;

      DLOG_F(INFO, "Requesting data from object %s", objectName);
      with_object(objectName, statusCode, [&](QObject * obj) {
          DLOG_F(INFO, "Object found %s", objectName);

          const QMetaObject* metaObject = obj->metaObject();

          // Add regular properties
          for(auto i = 0; i < metaObject->propertyCount(); ++i) {
            add_property_to_response( resp, "meta"
                , metaObject->property(i).name()
                , metaObject->property(i).read( obj ));
          }


          // Add dynamic properties
          for (auto qbaPropertyName : obj->dynamicPropertyNames() ) {
            auto propertyName = qbaPropertyName.constData();
            add_property_to_response( resp, "dynamic"
                , propertyName
                ,  obj->property( propertyName ));
          }

          // list actions
          for (auto action : ((QWidget *)obj)->actions() ) {
            resp[ "actions" ].push_back({
                {"text", qPrintable(action->text())},
                {"isVisible", action->isVisible()}
                });

            trigger_script_command( action, isScriptCommand );
          }

          // get the method information
          const auto methods = get_qobject_method_metadata( metaObject );

          // output the method information
          for (const auto& meth : methods) {
            add_qobject_method_to_response(resp, meth);
          }

      });

      vaccine::send_json(nc, resp, statusCode);
  }
}



// HANDLER : qobject/grab ---------------------------------------------



namespace {

  // Handle screenshots
  void handleQobjectGrab( const char* objectName,  struct mg_connection* nc, nlohmann::json& resp) {
      int statusCode = 200;

      // TODO: implement content-type type check
      with_object(objectName, statusCode, [&](QWidget * obj) {
          QByteArray bytes;
          QBuffer buffer(&bytes);
          buffer.open(QIODevice::WriteOnly);
          obj->grab().save(&buffer, "PNG");

          mg_send_head(nc, 200, bytes.length(), "Content-Type: image/png");
          mg_send(nc, bytes.constData() ,bytes.length());
          });

      if ( statusCode == 200 ) {
        return;
      } else {
        // we didn't find widget like this
        resp["error"] = "Widget not found";
        vaccine::send_json(nc,resp,statusCode);
        return;
      }
  }
}








// HANDLER : GET /qwidgets ---------------------------------------------
namespace {
  void get_all_qwidgets(mg_connection* nc)
  {
    nlohmann::json resp;
    char buf[40];

    LOG_SCOPE_FUNCTION(INFO);

    sprintf(buf,"%p",qApp);

    resp["qApp"] = buf;
    resp["appName"] = qPrintable(QApplication::applicationName());

    // All widgets
    for( QWidget * child : qApp->allWidgets() ) {
      if (child == nullptr) continue;
      const QMetaObject* metaObject = child->metaObject();

      resp["widgets"].push_back( {
            {"objectName",  qPrintable(child->objectName())},
            {"address", address_to_string(child)},
            {"parentName", child->parent() ? qPrintable(child->parent()->objectName()) : "" },
            {"obectKind", "widget" },
            {"className", child->metaObject()->className() },
            {"superClass", child->metaObject()->superClass()->className() }
      });
    }


    vaccine::send_json(nc, resp, 200);
  }

}

//
// HANDLER : qobject/grab ---------------------------------------------



namespace {


  bool requestHasClick(const nlohmann::json& req, QAbstractButton* obj) {
      return req["body"]["click"];
  }


  // Click the given button if its a QAbstractButton and  `pred` is true.
  // Returns true if the button was clicked, false otherwise
  template <typename ShouldClick>
  bool click_button_if(const nlohmann::json& req, QObject* obj, const ShouldClick pred) {
    // try to cast to QAbstractButton
    auto* btn = qobject_cast<QAbstractButton*>(obj);
    // then check the predicate
    if (btn != nullptr && pred(req, btn)) {
      btn->click();
      return true;
    }
    return false;
  }



  void set_properties_for_qobject(const char* objectName, const nlohmann::json& req, struct mg_connection* nc ) {
      nlohmann::json resp;
      int statusCode = 200;
      // set QWidget property (setPropert). Request type should be PATCH, but POST is accepted
      //
      // TODO: write tests
      with_object(objectName, statusCode, [&](QObject * obj) {
          DLOG_F(INFO, "Setting properties for object %s", objectName);

          for (auto& prop : req["body"]["properties"]) {
            obj->setProperty(
                prop["name"].get<std::string>().c_str(),
                prop["value"].get<std::string>().c_str());
          }

          // if we can click the button, click it then add its address
          // to the clicked list
          if (click_button_if( req, obj, requestHasClick )) {
            resp["clicked"].push_back({address_to_string(obj)});
          }
      });

      vaccine::send_json(nc,resp,statusCode);
  }
}






// PUBLIC
//
namespace vaccine {

  void qwidget_handler(
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
    if ( uri == "qwidgets" && is_request_method(hm,"GET") ) {
      return get_all_qwidgets(nc);
    } else if ( splitURI.size() == 2 && is_request_method(hm,"GET") ) {
      return list_qobject_data_for_object_named( objectName, nc);
    } else if ( splitURI.size() == 2 && !is_request_method(hm,"GET") ) {
      return set_properties_for_qobject(objectName, req, nc);
    } else if (uri == "qobject/grab") {
      return handleQobjectGrab( objectName,  nc, resp );
    } else {
      statusCode = 404;
      resp["error"] = "Method not found";
    }

    send_json(nc,resp,statusCode);
  }

  __attribute__((constructor))
    static void initializer(void) {
      DLOG_F(INFO, "Register qwidget service");
      vaccine::register_callback("qwidgets", qwidget_handler, qwidget_json);
    }

}

#endif // HAVE QT5CORE && QT5WIDGES
