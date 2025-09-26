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
	using finalizer_t = std::function<int(VI_TM_HJOUR h)>;
	int finalizer_default (vi_tmMeasurementsJournal_t *h)
	{	verify(0 <= vi_tmReportCb("Timing report:\n"));
		return vi_tmReport(h, vi_tmReportDefault, vi_tmReportCb);
	};

	class timing_global_t
	{	mutable std::mutex mtx_;
		std::size_t initialization_cnt_{ 0 };
		VI_TM_HJOUR const journal_{ vi_tmJournalCreate() };
		finalizer_t finalizer_{ finalizer_default };
		static std::unique_ptr<timing_global_t> global_instance_;

		timing_global_t() = default;
		timing_global_t(timing_global_t&) = delete;
		timing_global_t& operator()(timing_global_t&) = delete;
	public:
		static timing_global_t* global_instance(bool from_init = false);
		~timing_global_t();
		VI_TM_RESULT init(const char *title, VI_TM_FLAGS report_flags);
		VI_TM_RESULT finit();
		VI_TM_HJOUR handle() const noexcept { return journal_; }
		finalizer_t finalizer(finalizer_t fn) noexcept;
	};
}

timing_global_t::~timing_global_t()
{	std::lock_guard lg{ mtx_ };
#if !VI_TM_SHARED
	// This destructor will be called from DllMain. assert() may lead to program termination.
	assert(0 == initialization_cnt_ && "Every vi_tmInit call must have a corresponding vi_tmShutdown call.");
#endif
	if (verify(journal_))
	{	if (finalizer_)
		{	verify(finalizer_(journal_) >= 0);
		}
		
		vi_tmJournalClose(journal_);
	}
}

VI_TM_RESULT timing_global_t::init(const char *title, VI_TM_FLAGS flags)
{	std::lock_guard lg{mtx_};

	++initialization_cnt_;
	std::string&& t = title ? title : "";
	finalizer_ = [t = std::move(t), flags](VI_TM_HJOUR h)
		{	int result = VI_EXIT_SUCCESS;
			if (!t.empty() && vi_tmReportCb(t.c_str()) < 0)
			{	result = VI_EXIT_FAILURE;
			}
			else if (vi_tmReport(h, flags) < 0)
			{	result = VI_EXIT_FAILURE;
			}
			return result;
		};

	return VI_EXIT_SUCCESS;
}

VI_TM_RESULT timing_global_t::finit()
{	std::lock_guard lg{mtx_};

	if (verify(initialization_cnt_ > 0) && (0 == --initialization_cnt_) && verify(!!journal_))
	{	if (finalizer_)
		{	finalizer_(journal_);
		}
		finalizer_ = nullptr;
		vi_tmJournalReset(journal_);
	}
	return VI_EXIT_SUCCESS;
}

timing_global_t* timing_global_t::global_instance(bool from_init)
{	bool novel = false;
	static auto const global = [&novel]
		{	assert(!global_instance_);
			auto result = new(std::nothrow) timing_global_t;
			global_instance_.reset(result);
			novel = true;
			return result;
		}();

	// If vi_tmInit is called, it should only be called first, before any other global timer calls.
	assert(!from_init || novel || global->initialization_cnt_ > 0);
	return global;
}

finalizer_t timing_global_t::finalizer(finalizer_t fn) noexcept
{	std::lock_guard lg{ mtx_ };
	return std::exchange(finalizer_, std::move(fn));
}

VI_TM_RESULT VI_TM_CALL vi_tmGlobalSetReporter(const char *title, VI_TM_FLAGS flags)
{	if (auto const global = timing_global_t::global_instance())
	{	std::string&& t = title ? title : "";
		auto fn = [t = std::move(t), flags](VI_TM_HJOUR jnl)
			{	VI_TM_RESULT result = VI_EXIT_SUCCESS;
				if (!t.empty() && vi_tmReportCb(t.c_str()) < 0)
				{	result = VI_EXIT_FAILURE;
				}
				else if (vi_tmReport(jnl, flags) < 0)
				{	result = VI_EXIT_FAILURE;
				}
				return result;
			};
		global->finalizer(std::move(fn));
		return VI_EXIT_SUCCESS;
	}
	return VI_EXIT_FAILURE;
}

vi_tmMeasurementsJournal_t* misc::from_handle(VI_TM_HJOUR h)
{	if (VI_TM_HGLOBAL == h)
	{	static vi_tmMeasurementsJournal_t* const global_journal = []
			{	timing_global_t* const instance = timing_global_t::global_instance();
				return verify(instance) ? instance->handle() : nullptr;
			}();
		return global_journal;
	}
	assert(h);
	return h;
}

VI_TM_RESULT VI_TM_CALL vi_tmInit(const char *title, VI_TM_FLAGS report_flags, VI_TM_FLAGS flags)
{	VI_TM_RESULT result = VI_EXIT_FAILURE;

	assert(0 == (~static_cast<VI_TM_FLAGS>(vi_tmReportFlagsMask) & report_flags));
	if (auto const global = timing_global_t::global_instance(true))
	{	result = global->init(title, report_flags & vi_tmReportFlagsMask);
	}

	assert(0 == (~static_cast<VI_TM_FLAGS>(vi_tmInitFlagsMask) & flags));
	if (vi_tmInitWarmup & flags)
	{	vi_WarmUp();
	}
	if (vi_tmInitThreadYield & flags)
	{	vi_ThreadYield();
	}
	return result;
}

void VI_TM_CALL vi_tmShutdown()
{	if (auto const global = timing_global_t::global_instance(); verify(!!global))
	{	global->finit();
	}
}

#ifdef _MSC_VER
#	pragma warning(suppress: 4073) // WRN:"initializers put in library initialization area"
#	pragma init_seg(lib) // Objects in this group are constructed after the ones marked as compiler, but before any others.
#else
	[[gnu::init_priority(200)]] // init_priority range is 101..65535
#endif
	std::unique_ptr<timing_global_t> timing_global_t::global_instance_;
