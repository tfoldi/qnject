#include <stdio.h>
#include <unistd.h>
#include <thread>

// shutdown flag for the service thread
extern bool vaccine_shutdown;

extern void start_thread(); 

// standard thread handler
std::thread * service_thread;

// fwd decl
void start_thread();

// Initializer.
__attribute__((constructor))
  static void initializer(void) {  
    printf("[%s] Starting service thread\n", __FILE__);
    vaccine_shutdown = false;
    service_thread = new std::thread( start_thread );
  }

// Finalizer.
__attribute__((destructor))
  static void finalizer(void) {                               // 3
    printf("[%s] stopping service thread()\n", __FILE__);
    vaccine_shutdown = true;
    service_thread->join();
    delete service_thread;
  }

