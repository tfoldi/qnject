#include "mainwindow.h"
#include <QApplication>
#include <thread>

#define CATCH_CONFIG_RUNNER
#include "../../../deps/catch/catch.hpp"

struct catch_result { bool is_ready; int ret; } catch_result  = {false, 0};

void start_catch( int argc, char* const argv[] )
{
  int result = Catch::Session().run( argc, argv );

  catch_result = {true, result};
}

int main(int argc, char *argv[])
{
  std::thread test_thread;
  
  // running the Qt show
  QApplication a(argc, argv);
  MainWindow w;
  w.show();
  
  // start tests in separate thread if we have RUN_TESTS defined
  if (getenv("RUN_TESTS")) 
    test_thread = std::thread(start_catch,argc,argv);

  // start application
  int ret = a.exec();

  // join catch thread if joinable
  if (test_thread.joinable())
    test_thread.join();

  return (ret);
}
