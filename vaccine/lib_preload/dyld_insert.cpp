#include <stdio.h>
#include <unistd.h>
#include <thread>

#include "../vaccine.h"

// standard thread handler
static std::thread * service_thread;

// fwd decl
void start_thread();

// Initializer.
__attribute__((constructor))
  static void initializer(void) {
    printf("[%s] Starting service thread\n", __FILE__);
    vaccine::shutdown = false;
    service_thread = new std::thread( vaccine::start_thread );
  }

// Finalizer.
__attribute__((destructor))
  static void finalizer(void) {
    printf("[%s] stopping service thread()\n", __FILE__);
    vaccine::shutdown = true;
    service_thread->join();
    delete service_thread;
  }

