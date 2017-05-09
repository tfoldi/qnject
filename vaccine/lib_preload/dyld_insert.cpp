#include "qnject_config.h"

#include <stdio.h>
#include "dyld-hooking.h"

#ifndef _MSC_VER
#include <unistd.h>
#include <process.h>
#else

#endif
#include <thread>
#include "../../deps/loguru/loguru.hpp"

#include "../vaccine.h"
#include "loguru.hpp"


// fwd decl
void start_thread();



namespace {
	//// standard thread handler
	//// Initializer.
	//void dyld_insert_initializer(void) {
	//	DLOG_F(INFO, "Starting service thread");
	//	vaccine::state = vaccine::mg_state::INITALIZING;
	//	//vaccine::start_thread();
	//	service_thread = new std::thread(vaccine::start_thread);
	//	DLOG_F(INFO, "Started service thread");
	//}

	//// Finalizer.
	//void dyld_insert_finalizer(void) {
	//	DLOG_F(INFO, "Stopping service thread()");
	//	vaccine::state = vaccine::mg_state::SHUTDOWN_REQUESTED;
	//	service_thread->join();
	//	delete service_thread;
	//}
    std::thread* service_thread = nullptr;
#define INVALID_THREAD_HANDLE 0x1337
	HANDLE hThread = (void*)INVALID_THREAD_HANDLE;

	unsigned __stdcall SecondThreadFunc(void* pArguments)
	{
		printf("In second thread...\n");
		vaccine::start_thread();

		return 0;
	}

	struct dyld_insert_t {
		dyld_insert_t() {
			if (!service_thread) {
				printf("Staring service thread\n");
				//DLOG_F(INFO, "Starting service thread");
				vaccine::state = vaccine::mg_state::INITALIZING;
#ifdef _MSC_VER
				unsigned threadID;
				if (uintptr_t(hThread) == INVALID_THREAD_HANDLE) {
					hThread = (HANDLE)_beginthreadex(NULL, 0, &SecondThreadFunc, NULL, 0, &threadID);
				}
					// Create the second thread.
#endif
				//vaccine::start_thread();
				//service_thread = new std::thread(vaccine::start_thread);
				printf("Started service thread\n");
				//DLOG_F(INFO, "Started service thread");

			}
		}
		~dyld_insert_t() {
			if (service_thread) {
				DLOG_F(INFO, "Stopping service thread()");
				vaccine::state = vaccine::mg_state::SHUTDOWN_REQUESTED;
				//service_thread->join();
			}
		}


	};



	dyld_insert_t onSpawn;
}


// DLL_INITILAIZER_AND_FINALIZER(dyld_insert_initializer, dyld_insert_finalizer);


