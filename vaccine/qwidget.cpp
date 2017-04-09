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

#include "../deps/loguru/loguru.hpp"
#include "../deps/json/json.hpp"

#include "utils.hpp"
#include "vaccine.h"
#include "qwidget.json.h"


// INTERNAL
//
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


  template <typename Json>
  void add_qobject_method_to_response(Json& resp, const QObjectMethod& meth) {
      nlohmann::json local;
      local["name"] = meth.name;
      local["type"] = meth.type;
      local["access"] = meth.accessKind;
      local["signature"] = meth.signature;
      resp["methods"].push_back(local);
  }

}


// PUBLIC
//
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

  void get_all_qwidgets(nlohmann::json const & req, nlohmann::json & resp)
  {
    char buf[40];

    LOG_SCOPE_FUNCTION(INFO);

    sprintf(buf,"%p",qApp);

    resp["qApp"] = buf;
    resp["appName"] = qPrintable(QApplication::applicationName());

    // All widgets
    for( QWidget * child : qApp->allWidgets() ) {
      if (child && child->objectName() != "")  {
        const QMetaObject* metaObject = child->metaObject();

        // get the method information
        const auto methodList = get_qobject_method_metadata( metaObject );

        nlohmann::json methods;
        // output the method information
        for (const auto& meth : methodList) {
          add_qobject_method_to_response(methods, meth);
        }
        resp["widgets"].push_back(
            {
              {"objectName",  qPrintable(child->objectName())},
              {"parentName", child->parent() ? qPrintable(child->parent()->objectName()) : "" },
              {"obectKind", "widget" },
              {"className", child->metaObject()->className() },
              {"superClass", child->metaObject()->superClass()->className() },
              {"methods", methods }
            }
            );
      }
    }
  }

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
      get_all_qwidgets(req,resp);
    } else if ( splitURI.size() == 2 && is_request_method(hm,"GET") ) {
      DLOG_F(INFO, "Requesting data from object %s", objectName);
      with_object(objectName, statusCode, [&](QObject * obj) {
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

          // get the method information
          const auto methods = get_qobject_method_metadata( metaObject );

          // output the method information
          for (const auto& meth : methods) {
            add_qobject_method_to_response(resp, meth);
            // resp[ "methods" ].push_back( { propertyName, qPrintable(val.toString()), "dynamic" } );
          }

          });
    } else if (uri == "qobject/getProperty") {
      with_object(objectName, statusCode, [&](QObject * obj) {
          QVariant val = obj->property( req["name"].get<std::string>().c_str() );
          resp[ "name" ] = qPrintable(val.toString()) ;
          });
    } else if (uri == "qobject/setProperty") {
      with_object(objectName, statusCode, [&](QObject * obj) {
          obj->setProperty( req["name"].get<std::string>().c_str(), req["value"].get<std::string>().c_str());
          });
    } else if (uri == "qobject/grab") {
      // check if we found our widget
      with_object(objectName, statusCode, [&](QWidget * obj) {
          QByteArray bytes;
          QBuffer buffer(&bytes);
          buffer.open(QIODevice::WriteOnly);
          obj->grab().save(&buffer, "PNG");

          mg_send_head(nc, 200, bytes.length(), "Content-Type: image/png");
          mg_send(nc, bytes.constData() ,bytes.length());
          });

      if ( statusCode == 200 ) {
        // we already sent back the reply, nothing to do
        return;
      } else {
        // we didn't find widget like this
        resp["error"] = "Widget not found";
      }
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
