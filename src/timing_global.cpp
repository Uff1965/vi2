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

#include <vi_timing/vi_timing.h>
#include "version.h" // For build number generation.
#include "misc.h"

#include <cassert>
#include <functional>
#include <memory>
#include <mutex>

namespace
{
	class timing_global_t
	{	static int finalizer_default(VI_TM_HJOUR h)
		{	verify(0 <= vi_tmReportCb("Timing:\n"));
			return vi_tmReport(h, vi_tmShowResolution | vi_tmShowDuration | vi_tmSortByTime, vi_tmReportCb);
		};
		using finalizer_t = std::function<int(vi_tmMeasurementsJournal_t *)>;

		VI_TM_HJOUR const handle_{ vi_tmJournalCreate() };
		finalizer_t finalizer_{ finalizer_default };
		mutable std::mutex mtx_;
		static std::unique_ptr<timing_global_t> global_instance_;
		timing_global_t() = default;
		timing_global_t(timing_global_t&) = delete;
		timing_global_t& operator()(timing_global_t&) = delete;
	public:
		~timing_global_t();
		static timing_global_t* global_instance();

		timing_global_t(const VI_TM_HJOUR handle_, const std::function<int(vi_tmMeasurementsJournal_t *)> &finalizer_)
			: handle_(handle_), finalizer_(finalizer_)
		{}

		VI_TM_HJOUR handle() const noexcept { return handle_; }
		finalizer_t finalizer() const noexcept { std::lock_guard lg{ mtx_ }; return finalizer_; }
		finalizer_t finalizer(finalizer_t fn) noexcept { std::lock_guard lg{ mtx_ }; return std::exchange(finalizer_, std::move(fn)); }
	};
}

timing_global_t::~timing_global_t()
{	std::lock_guard lg{ mtx_ };
	if (verify(handle_))
	{	if (finalizer_)
		{	finalizer_(handle_);
		}
		
		vi_tmJournalClose(handle_);
	}
}

timing_global_t* timing_global_t::global_instance()
{
	static auto const global = []
		{	assert(!global_instance_);
			global_instance_.reset(new(std::nothrow) timing_global_t);
			return verify(!!global_instance_)? global_instance_.get(): nullptr;
		}();

	return global;
}

VI_TM_RESULT VI_TM_CALL vi_tmGlobalReporter(VI_TM_RESULT (*cb)(VI_TM_HJOUR, void *), void *ctx)
{	assert(cb);
	if (auto const global = timing_global_t::global_instance())
	{	global->finalizer([cb, ctx](VI_TM_HJOUR jnl) { return !!cb ? cb(jnl, ctx) : 0; });
	}
	return VI_EXIT_SUCCESS;
}

VI_TM_RESULT VI_TM_CALL vi_tmGlobalReporterPrn
(	const char *title,
	VI_TM_FLAGS flags,
	vi_tmReportCb_t cb,
	void *ctx
)
{	assert(!ctx || !!cb);

	if (auto const global = timing_global_t::global_instance())
	{	std::string&& t = title ? title : "";
		auto fn = [t, flags, cb, ctx](VI_TM_HJOUR jnl)
			{	if (!!cb)
				{	VI_TM_RESULT result = 0;
					if (!t.empty())
					{	result += cb(t.c_str(), ctx);
					}
					result += vi_tmReport(jnl, flags, cb, ctx);
					return result;
				}
				return 0;
			};
		global->finalizer(std::move(fn));
	}
	return VI_EXIT_SUCCESS;
}

vi_tmMeasurementsJournal_t* misc::from_handle(VI_TM_HJOUR handle)
{	if (VI_TM_HGLOBAL == handle)
	{	static const auto global = []
			{	const auto instance = timing_global_t::global_instance();
				return verify(instance) ? instance->handle() : nullptr;
			}();
		return global;
	}

	assert(handle);
	return handle;
}

VI_TM_RESULT VI_TM_CALL vi_tmInit(const char *title, VI_TM_FLAGS report_flags, VI_TM_FLAGS flags)
{	assert(0 == (~vi_tmReportFlagsMask & report_flags));
	assert(0 == (~vi_tmInitFlagsMask & flags));
	if (vi_tmInitWarmup & flags)
	{	vi_WarmUp();
	}
	const auto result = vi_tmGlobalReporterPrn(title, report_flags & vi_tmReportFlagsMask);
	if (vi_tmInitThreadYield & flags)
	{	vi_ThreadYield();
	}
	return result;
}

void VI_TM_CALL vi_tmShutdown()
{
}

#ifdef _MSC_VER
#	pragma warning(suppress: 4073) // WRN:"initializers put in library initialization area"
#	pragma init_seg(lib) // Objects in this group are constructed after the ones marked as compiler, but before any others.
#else
	[[gnu::init_priority(200)]] // init_priority range is 101..65535
#endif
	std::unique_ptr<timing_global_t> timing_global_t::global_instance_;
