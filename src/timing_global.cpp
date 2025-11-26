// This is an independent project of an individual developer. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: http://www.viva64.com

/*****************************************************************************\
* This file is part of the vi_timing library.
* 
* vi_timing - a compact, lightweight C/C++ library for measuring code 
* execution time. It was developed for experimental and educational purposes, 
* so please keep expectations reasonable.
*
* Report bugs or suggest improvements to author: <programmer.amateur@proton.me>
*
* LICENSE & DISCLAIMER:
* - No warranties. Use at your own risk.
* - Licensed under Business Source License 1.1 (BSL-1.1):
*   - Free for non-commercial use.
*   - For commercial licensing, contact the author.
*   - Change Date: 2029-09-01 - after which the library will be licensed 
*     under GNU GPLv3.
*   - Attribution required: "vi_timing Library (c) A.Prograamar".
*   - See LICENSE in the project root for full terms.
\*****************************************************************************/

#include "build_number_generator.h" // For build number generation.
#include "misc.h"
#include <vi_timing/vi_timing.h>

#include <cassert>
#include <functional>
#include <memory>
#include <mutex>

namespace
{
	class global_registry_t final
	{	using finalizer_t = std::function<int(const global_registry_t&)>;

		mutable std::mutex mtx_;
		VI_TM_HREG const handle_ = vi_tmRegistryCreate();
		finalizer_t finalizer_ = make_finalizer("Timing report:\n", "", vi_tmReportDefault);
		static std::unique_ptr<global_registry_t> instance_owner_; // Global timing instance - early initialization, late destruction. See implementation for platform-specific initialization details.

		global_registry_t() = default;
		global_registry_t(const global_registry_t&) = delete;
		global_registry_t& operator=(const global_registry_t &) = delete;
	public:
		~global_registry_t();
		static global_registry_t* instance();
		VI_TM_HREG handle() const noexcept { return handle_; }
		finalizer_t set_finalizer(finalizer_t fn) noexcept;
		static finalizer_t make_finalizer(std::string title, std::string footer, VI_TM_FLAGS flags);
	};
}

global_registry_t::~global_registry_t()
{	std::lock_guard lg{ mtx_ };
	if (verify(handle_))
	{	if (finalizer_)
		{	verify(VI_SUCCEEDED(finalizer_(*this)));
		}
		vi_tmRegistryClose(handle_);
	}
}

global_registry_t::finalizer_t global_registry_t::make_finalizer(
	std::string header,
	std::string footer,
	VI_TM_FLAGS flags
)
{	return [h = std::move(header), f = std::move(footer), flags](const global_registry_t &registry)
		{	if ((flags & vi_tmDoNotReport) == 0)
			{	if (!h.empty() && VI_FAILED(vi_tmReportCb(h.c_str()))) 
				{	return VI_FAILURE;
				}
				if (VI_FAILED(vi_tmRegistryReport(registry.handle(), flags)))
				{	return VI_FAILURE;
				}
			}
			if (!f.empty() && VI_FAILED(vi_tmReportCb(f.c_str())))
			{	return VI_FAILURE;
			}
			return VI_SUCCESS;
		};
}

global_registry_t::finalizer_t global_registry_t::set_finalizer(finalizer_t fn) noexcept
{	std::lock_guard lg{ mtx_ };
	return std::exchange(finalizer_, std::move(fn));
}

global_registry_t* global_registry_t::instance()
{	static auto const global = []
		{	instance_owner_.reset(new(std::nothrow) global_registry_t);
			return instance_owner_.get();
		}();
	return global;
}

vi_tmRegistry_t* misc::from_handle(VI_TM_HREG h)
{	if (VI_TM_HGLOBAL == h)
	{	static vi_tmRegistry_t* const hglobal = []
			{	auto const instance = global_registry_t::instance();
				return verify(instance) ? instance->handle() : nullptr;
			}();
		return hglobal;
	}
	assert(h);
	return h;
}

VI_TM_RESULT VI_TM_CALL vi_tmGlobalInit(VI_TM_FLAGS flags, const char *title, const char *footer)
{	assert((flags & ~vi_tmReportFlagsMask) == 0);
	if (auto const global = global_registry_t::instance())
	{	std::string t = title ? title : "Timing report:\n";
		std::string f = footer ? footer : "";
		global->set_finalizer(global_registry_t::make_finalizer(std::move(t), std::move(f), flags));
		return VI_SUCCESS;
	}
	return VI_FAILURE;
}

/*
 * Global instance of global_registry_t.
 * 
 * LIFECYCLE SEMANTICS:
 * 
 * INITIALIZATION: Created in special initialization section:
 * - Windows: "lib" section (after "compiler" but before "user")
 * - Linux/GCC: with priority 200 (higher than standard 65535)
 * Guarantees initialization BEFORE most static objects.
 * 
 * DESTRUCTION: Destroyed in REVERSE initialization order:
 * - After most static objects
 * - Before global library mechanisms cleanup
 * 
 * PURPOSE: Ensure proper lifetime for global timing measurements,
 * making it available during initialization/destruction of other static objects.
 * 
 * IMPORTANT: Order is explicit and link-order independent.
 */
#if !VI_TM_SHARED
#	ifdef _MSC_VER
#		if defined(_DEBUG) && !defined(_DLL)
#			pragma message(__FILE__ "(" VI_STRINGIZE(__LINE__) "): warning: "
				"Static debug CRT (/MTd) destroys locale and iostream objects "
				"before .CRT$XIL (init_seg(lib)) objects are finalized. "
				"Global deinitialization order may be incorrect; init_seg(lib) disabled.")
//#			pragma warning(suppress: 4073)
//#			pragma init_seg(lib)
#		else
#			pragma warning(suppress: 4073)
#			pragma init_seg(lib)
#		endif
#		define VI_HIINITPRIORITY
#	else
#		define VI_HIINITPRIORITY [[gnu::init_priority(200)]]
#	endif
#else
#	define VI_HIINITPRIORITY
#endif

VI_HIINITPRIORITY std::unique_ptr<global_registry_t> global_registry_t::instance_owner_;
