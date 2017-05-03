#include "qnject_config.h"

#include <stdio.h>
#include "dyld-hooking.h"

#ifndef _MSC_VER
#include <unistd.h>

#else

#endif
#include <thread>
#include "../../deps/loguru/loguru.hpp"

#include "../vaccine.h"

// standard thread handler
static std::thread * service_thread;

// fwd decl
void start_thread();

// Initializer.
  static void dyld_insert_initializer(void) {
    DLOG_F(INFO,"Starting service thread");
    vaccine::state = vaccine::mg_state::INITALIZING;
    service_thread = new std::thread( vaccine::start_thread );
  }

// Finalizer.
  static void dyld_insert_finalizer(void) {
    DLOG_F(INFO, "Stopping service thread()");
    vaccine::state = vaccine::mg_state::SHUTDOWN_REQUESTED;
    service_thread->join();
    delete service_thread;
  }






DLL_INITILAIZER_AND_FINALIZER(dyld_insert_initializer, dyld_insert_finalizer);