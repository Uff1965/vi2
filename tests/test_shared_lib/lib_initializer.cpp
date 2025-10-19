// This is an independent project of an individual developer. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: http://www.viva64.com

#if defined(_WIN32)
#	include <windows.h>
#endif

#include <vi_timing/vi_timing.hpp>

#if defined(_WIN32)
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

	default:
		assert((DLL_THREAD_ATTACH == ul_reason_for_call) || (DLL_THREAD_DETACH == ul_reason_for_call));
		break;
	}
	return TRUE;
}
#else
__attribute__((constructor)) void init();
__attribute__((destructor)) void cleanup();
#endif

void init()
{
}

void cleanup()
{
}
