#include "../../config.h"

#include <stdio.h>
#include <unistd.h>
#include <thread>

#include "../../deps/loguru/loguru.hpp"

#include "../vaccine.h"

// standard thread handler
static std::thread * service_thread;

// fwd decl
void start_thread();

// Initializer.
__attribute__((constructor))
  static void initializer(void) {
    DLOG_F(INFO,"Starting service thread");
    vaccine::state = vaccine::mg_state::INITALIZING;
    service_thread = new std::thread( vaccine::start_thread );
  }

// Finalizer.
__attribute__((destructor))
  static void finalizer(void) {
    DLOG_F(INFO, "Stopping service thread()");
    vaccine::state = vaccine::mg_state::SHUTDOWN_REQUESTED;
    service_thread->join();
    delete service_thread;
  }

