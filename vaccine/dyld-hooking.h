#pragma once

#ifdef _MSC_VER

#define DLL_INITILAIZER_AND_FINALIZER(initializer, finalizer) \
\
BOOL APIENTRY DllMain(HMODULE hModule, DWORD  ul_reason_for_call, LPVOID lpReserved) { \
	switch (ul_reason_for_call) { \
	case DLL_PROCESS_ATTACH: initializer(); break; \
	case DLL_THREAD_ATTACH: printf("DllMain, DLL_THREAD_ATTACH\n"); break; \
	case DLL_THREAD_DETACH: printf("DllMain, DLL_THREAD_DETACH\n"); break; \
	case DLL_PROCESS_DETACH: finalizer(); break; \
	default: printf("DllMain, ????\n"); break; \
	} \
	return TRUE; \
}

#else
// Initializer.

#define DLL_INITILAIZER_AND_FINALIZER(initializer, finalizer) \
\
__attribute__((constructor)) \
static void dyld_initializer(void) { initializer(); } \
\
__attribute__((destructor)) \
static void dyld_finalizer(void) { finalizer(); } \





#endif

