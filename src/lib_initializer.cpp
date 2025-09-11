#if defined(_WIN32)
#include <windows.h>
#else
#include <dlfcn.h>
#endif

#include <cassert>
#include <cstddef>

#if defined(_MSC_VER)
void init();
void cleanup();

BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved)
{	(void)hModule;
	(void)lpReserved;

	switch (ul_reason_for_call)
	{
	case DLL_PROCESS_ATTACH:
		init();
		break;

	case DLL_PROCESS_DETACH:
		cleanup();
		break;

	case DLL_THREAD_ATTACH:
	case DLL_THREAD_DETACH:
		break;

	default:
		assert(false && "Uknown ul_reason_for_call!");
		break;
	}
	return TRUE;
}
#elif defined(_linu) || defined(__clang__)
__attribute__((constructor)) void init();
__attribute__((destructor)) void cleanup();
#endif

void init()
{
}

void cleanup()
{
}
