#if defined(_WIN32)
#	include <windows.h>
#else
#	include <dlfcn.h>
#endif

#include <cassert>
#include <cstddef>
#include <type_traits>

#if defined(_MSC_VER) || defined(__MINGW32__)
#	if defined(_M_IX86) || defined(__i386__)
#		define HOOK_CC __cdecl
#	else
#		define HOOK_CC
#	endif
#else
#	define HOOK_CC
#endif

namespace
{
	using PFV = void (HOOK_CC *) (void) noexcept;

	void HOOK_CC init(void) noexcept
	{/**/
	}
	static_assert(std::is_same_v<PFV, decltype(&init)>);

	void HOOK_CC cleanup(void) noexcept
	{/**/
	}
	static_assert(std::is_same_v<PFV, decltype(&cleanup)>);
}

#if defined(_MSC_VER)
#	pragma section(".CRT$XCU", read)
#	pragma section(".CRT$XPU", read)

extern "C"
{
	// .CRT$XCU: executed before main/DllMain, before global constructors.
	extern __declspec(allocate(".CRT$XCU")) const PFV p_init_hook = init;
	// .CRT$XPU — called when the module is unloaded, after global destructors and atexit() functions.
	extern __declspec(allocate(".CRT$XPU")) const PFV p_cleanup_hook = cleanup;
}

#	if defined(_M_IX86)
#		pragma comment(linker, "/include:_p_init_hook") // For x86, the symbol names have a leading underscore.
#		pragma comment(linker, "/include:_p_cleanup_hook") 
#	else
#		pragma comment(linker, "/include:p_init_hook") // For x64 and ARM, no leading underscore.
#		pragma comment(linker, "/include:p_cleanup_hook")
#	endif

#elif defined(__GNUC__)

__attribute__((constructor(101)))
void init_wrapper(void)
{
	init();
}

__attribute__((destructor(65000)))
void cleanup_wrapper(void)
{
	cleanup();
}

#else
#	warning "Portable init/cleanup-hooks are not supported on this compiler"
#endif  // Compiler branches
