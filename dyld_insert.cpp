#include <stdio.h>
#include <unistd.h>
#include <thread>
#include <QCoreApplication>

// shutdown flag for the service thread
static bool shutdown = false;

// standard thread handler
std::thread * service_thread;

// fwd decl
void start_thread();

// Initializer.
__attribute__((constructor))
  static void initializer(void) {  
    printf("[%s] Starting service thread\n", __FILE__);
    service_thread = new std::thread( start_thread );
  }

// Finalizer.
__attribute__((destructor))
  static void finalizer(void) {                               // 3
    printf("[%s] stopping service thread()\n", __FILE__);
    shutdown = true;
    service_thread->join();
    delete service_thread;
  }

void start_thread() {
  printf("thread started, looking for qApp\n");
  
  // search for QApplication
  while ( !shutdown && !qApp ) 
    usleep(250000);
  

  printf("got qApp = %p\n", qApp );
}
