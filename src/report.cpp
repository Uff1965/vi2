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
*   - Attribution required: “vi_timing Library © A.Prograamar”.
*   - See LICENSE in the project root for full terms.
\*****************************************************************************/

#include <vi_timing/vi_timing.h>
#include "version.h"
#include "misc.h"

#include <algorithm>
#include <cassert>
#include <cmath>
#include <iomanip>
#include <numeric>
#include <sstream>
#include <string>
#include <tuple>
#include <vector>

#ifdef _WIN32
#	include <windows.h> // For GetStdHandle(STD_OUTPUT_HANDLE) and OutputDebugString.
#endif

using namespace std::literals;

namespace
{
	constexpr auto TitleNumber = "#"sv;
	constexpr auto TitleName = "Name"sv;
	constexpr auto TitleAverage = "Avg."sv;
	constexpr auto TitleTotal = "Total"sv;
	constexpr auto TitleCV = "CV"sv;
	constexpr auto TitleAmount = "Cnt."sv;
	constexpr auto TitleMin = "Min."sv;
	constexpr auto TitleMax = "Max."sv;
	constexpr auto Ascending = " (^)"sv;
	constexpr auto Descending = " (v)"sv;
	constexpr auto Insignificant = "<ins>"sv; // insignificant
	constexpr auto Excessive = "<exc>"sv; // excessively
	constexpr auto NotAvailable = ""sv;

	constexpr unsigned char DURATION_PREC = 2;
	constexpr unsigned char DURATION_DEC = 1;

	template<unsigned char P, unsigned char D>
	struct duration_t: std::chrono::duration<double> // A new type is defined to be able to overload the 'operator<'.
	{
		template<typename T>
		constexpr duration_t(T &&v): std::chrono::duration<double>{ std::forward<T>(v) } {}
		duration_t() = default;

		[[nodiscard]] friend std::string to_string(const duration_t &d) { return misc::to_string(d.count(), P, D) + "s"; }
		[[nodiscard]] friend bool operator<(const duration_t &l, const duration_t &r)
			{	return l.count() < r.count() && to_string(l) != to_string(r);
			}
	};

	struct metering_t
	{	std::string name_; // Name of the measured.
		std::size_t calls_{};
		std::size_t cnt_{}; // Number of measured units
		std::string cnt_txt_{ "0" };
		duration_t<DURATION_PREC, DURATION_DEC> sum_{}; // seconds
		std::string sum_txt_{ NotAvailable };
		duration_t<DURATION_PREC, DURATION_DEC> average_{}; // seconds
		std::string average_txt_{ NotAvailable };
#if VI_TM_STAT_USE_FILTER
		double cv_{}; // Coefficient of Variation.
		std::string cv_txt_{ NotAvailable }; // Coefficient of Variation (CV) in percent
#endif
#if VI_TM_STAT_USE_MINMAX
		duration_t<DURATION_PREC, DURATION_DEC> min_{}; // Minimum time in seconds
		std::string min_txt_{ NotAvailable };
		duration_t<DURATION_PREC, DURATION_DEC> max_{}; // Maximum time in seconds
		std::string max_txt_{ NotAvailable };
#endif

		metering_t(const char *name, const vi_tmMeasurementStats_t &meas, unsigned flags) noexcept;
	};

	template<vi_tmReportFlags_e E> auto make_tuple(const metering_t &v);

	template<> auto make_tuple<vi_tmSortByName>(const metering_t &v)
	{	return std::tie( v.name_, v.average_, v.sum_, v.cnt_ );
	}
	template<> auto make_tuple<vi_tmSortBySpeed>(const metering_t &v)
	{	return std::tie( v.average_, v.sum_, v.cnt_, v.name_ );
	}
	template<> auto make_tuple<vi_tmSortByTime>(const metering_t &v)
	{	return std::tie( v.sum_, v.average_, v.cnt_, v.name_ );
	}
	template<> auto make_tuple<vi_tmSortByAmount>(const metering_t &v)
	{	return std::tie( v.cnt_, v.average_, v.sum_, v.name_ );
	}

	template<vi_tmReportFlags_e E> bool less(const metering_t &l, const metering_t &r)
	{	return make_tuple<E>(l) < make_tuple<E>(r);
	}

	class comparator_t
	{	bool (*pr_)(const metering_t &, const metering_t &);
		const bool ascending_;
	public:
		explicit comparator_t(unsigned flags) noexcept
			: ascending_{ 0 != (flags & vi_tmSortAscending) }
		{
			switch (flags & vi_tmSortMask)
			{
			default:
				assert(false);
				[[fallthrough]];
			case vi_tmSortByName:
				pr_ = less<vi_tmSortByName>;
				break;
			case vi_tmSortByAmount:
				pr_ = less<vi_tmSortByAmount>;
				break;
			case vi_tmSortByTime:
				pr_ = less<vi_tmSortByTime>;
				break;
			case vi_tmSortBySpeed:
				pr_ = less<vi_tmSortBySpeed>;
				break;
			}
		}
		bool operator ()(const metering_t &l, const metering_t &r) const
		{	return ascending_ ? pr_(l, r): pr_(r, l);
		}
	};

	struct formatter_t
	{	static constexpr auto UNDERSCORE = '.';
		const std::size_t max_len_number_ = 0U;
		const unsigned flags_ = 0U;
		const unsigned guideline_interval_ = 0U;

		std::size_t max_len_name_{TitleName.length()};
		std::size_t max_len_average_{TitleAverage.length()};
#if VI_TM_STAT_USE_FILTER
		std::size_t max_len_cv_{TitleCV.length()};
#endif
#if VI_TM_STAT_USE_MINMAX
		std::size_t max_len_min_{TitleMin.length()};
		std::size_t max_len_max_{TitleMax.length()};
#endif
		std::size_t max_len_total_{TitleTotal.length()};
		std::size_t max_len_amount_{TitleAmount.length()};
		mutable std::size_t n_{ 0 };

		formatter_t(const std::vector<metering_t> &itms, unsigned flags);
		template<typename F>
		int print_header( const F &fn) const;
		template<typename F>
		int print_metering(const metering_t &i, const F &fn) const;

		std::size_t width_column(vi_tmReportFlags_e clmn) const;
		std::string item_column(vi_tmReportFlags_e clmn) const;
	};

	std::vector<metering_t> get_meterings(VI_TM_HJOUR journal_handle, unsigned flags)
	{	std::vector<metering_t> result;
		auto data = std::tie(result, flags);
		using data_t = decltype(data);
		vi_tmMeasurementEnumerate
		(	journal_handle,
			[](VI_TM_HMEAS h, void *callback_data)
			{	const char *name;
				vi_tmMeasurementStats_t meas;
				vi_tmMeasurementGet(h, &name, &meas);
				auto& [v, f] = *static_cast<data_t*>(callback_data); // The pointer to void is necessary for C compatibility.
				v.emplace_back(name, std::move(meas), f);
				return 0; // Ok, continue enumerate.
			},
			&data
		);
		return result;
	}

	template<typename F>
	int print_props(const F &fn, unsigned flags)
	{	int result = 0;
		if (flags & vi_tmShowMask)
		{	std::ostringstream str;
			auto &props = misc::properties_t::props();

			auto to_string = [](auto d) { return misc::to_string(d, DURATION_PREC, DURATION_DEC) + "s. "; };
			if (flags & vi_tmShowAux)
			{
#if VI_TM_THREADSAFE
				str << (flags & vi_tmDoNotSubtractOverhead? "": "Corrected; Thread-safe. ");
#else
				str << (flags & vi_tmDoNotSubtractOverhead? "": "Corrected. ");
#endif
			}
			if (flags & vi_tmShowResolution)
			{	str << "Resolution: " << to_string(props.seconds_per_tick_.count() * props.clock_resolution_ticks_);
			}
			if (flags & vi_tmShowDuration)
			{	str << "Duration: " << to_string(props.duration_threadsafe_.count());
			}
			if (flags & vi_tmShowDurationEx)
			{	str << "Duration ex: " << to_string(props.duration_ex_threadsafe_.count());
			}
			if (flags & vi_tmShowUnit)
			{	str << "One tick: " << to_string(props.seconds_per_tick_.count());
			}
			if (flags & vi_tmShowOverhead)
			{	str << "Overhead: " << to_string(props.seconds_per_tick_.count() * props.clock_overhead_ticks_);
			}

			str << '\n';
			result += fn(str.str().c_str());
		}

		return result;
	}

	vi_tmReportFlags_e to_sort_flag(unsigned flags_)
	{ // Convert flags_ to vi_tmReportFlags_e type, ensuring it is one of the defined sorting types.
		switch (auto s = flags_ & vi_tmSortMask)
		{
		case vi_tmSortByTime:
		case vi_tmSortByName:
		case vi_tmSortBySpeed:
		case vi_tmSortByAmount:
			return static_cast<vi_tmReportFlags_e>(s);

		default:
			assert(false);
			return vi_tmSortByName;
		}
	}

} // namespace

metering_t::metering_t(const char *name, const vi_tmMeasurementStats_t &meas, unsigned flags) noexcept
:	name_{ name }
{	
	if (!verify(VI_EXIT_SUCCESS == vi_tmMeasurementStatsIsValid(&meas)) || 0 == meas.calls_)
	{	return; // If the measurement is invalid or has no calls, we do not create a metering_t.
	}

	const auto &props = misc::properties_t::props();
	const auto correction_ticks = (0U == (flags & vi_tmDoNotSubtractOverhead)) ? props.clock_overhead_ticks_ : 0.0;

// calls_
	calls_ = meas.calls_;

// cnt_, sum_ and sum_txt_
#if VI_TM_STAT_USE_BASE
	cnt_ = meas.cnt_;
	try
	{	std::ostringstream str_stream;
		str_stream.imbue(std::locale(str_stream.getloc(), new misc::space_out));
		str_stream << cnt_;
		cnt_txt_ = str_stream.str();
	}
	catch (const std::exception &)
	{	assert(false);
	}

	const auto total_ticks = static_cast<double>(meas.sum_) - correction_ticks * static_cast<double>(meas.calls_); // Total time in ticks, corrected for overhead if necessary.
	if (total_ticks <= props.clock_resolution_ticks_ * std::sqrt(meas.calls_))
	{	sum_txt_ = Insignificant;
	}
	else
	{	sum_ = props.seconds_per_tick_ * total_ticks;
		sum_txt_ = to_string(sum_);
	}
#endif

// average, limit, cv_ and cv_txt_
#if VI_TM_STAT_USE_FILTER
	const auto limit_ticks = props.clock_resolution_ticks_ / std::sqrt(meas.flt_cnt_);
	const auto avg_ticks = meas.flt_avg_ - correction_ticks;

	if (meas.flt_calls_ >= 2) // To calculate the measurement spread, at least two measurements must be taken.
	{	assert(meas.flt_cnt_ >= static_cast<VI_TM_FP>(2)); // The first two measurements cannot be filtered out.
		cv_ = std::sqrt(meas.flt_ss_ / (meas.flt_cnt_ - static_cast<VI_TM_FP>(1))) / avg_ticks;
		if (const auto cv_pct = std::round(cv_ * 100.0); cv_pct < 1.0)
		{	cv_txt_ = "<1%"; // Coefficient of Variation (CV) is too low.
		}
		else if (cv_pct >= 100.0)
		{	cv_txt_ = Excessive; // Coefficient of Variation (CV) is too high.
		}
		else
		{	cv_txt_ = misc::to_string(cv_pct, 2, 0);
			assert(cv_txt_.back() == ' '); // In these conditions, the last character is always a space.
			assert(cv_txt_[cv_txt_.size() - 2] == ' ');
			cv_txt_.resize(cv_txt_.size() - 2);
			cv_txt_ += '%';
		}
	}
#elif VI_TM_STAT_USE_BASE
	const auto limit_ticks = props.clock_resolution_ticks_ / std::sqrt(static_cast<VI_TM_FP>(meas.cnt_));
	const auto avg_ticks = total_ticks / static_cast<double>(meas.cnt_);
#endif

// average_, average_txt_ and cnt_txt_
#if VI_TM_STAT_USE_BASE || VI_TM_STAT_USE_FILTER
	if (avg_ticks <= std::max(limit_ticks, props.clock_resolution_ticks_ * 1e-2))
	{	average_txt_ = Insignificant;
	}
	else
	{	average_ = props.seconds_per_tick_ * avg_ticks;
		average_txt_ = to_string(average_);
	}
#endif

// min_, max_, min_txt_ and max_txt_
#if VI_TM_STAT_USE_MINMAX
	if (meas.calls_ != 0U)
	{	// If there is more than one measurement, the minimum and maximum values are meaningful.
		if (const auto ticks = meas.min_ - correction_ticks; ticks <= props.clock_resolution_ticks_)
		{	min_txt_ = Insignificant;
		}
		else
		{	min_ = props.seconds_per_tick_ * ticks;
			min_txt_ = to_string(min_);
		}

		if (const auto ticks = meas.max_ - correction_ticks; ticks <= props.clock_resolution_ticks_)
		{	max_txt_ = Insignificant;
		}
		else
		{	max_ = props.seconds_per_tick_ * ticks;
			max_txt_ = to_string(max_);
		}
	}
#endif
}

formatter_t::formatter_t(const std::vector<metering_t> &itms, unsigned flags)
:	max_len_number_{ std::max(itms.empty() ? 1U : 1U + size_t(floor(log10(itms.size()))), std::size(TitleNumber) - 1U) },
	flags_{ flags },
	guideline_interval_{ itms.size() > 4U ? 3U : 0U }
{	
	for (auto &itm : itms)
	{	max_len_name_ = std::max(max_len_name_, itm.name_.length());
#if VI_TM_STAT_USE_BASE
		max_len_total_ = std::max(max_len_total_, itm.sum_txt_.length());
#endif
#if VI_TM_STAT_USE_FILTER
		max_len_cv_ = std::max(max_len_cv_, itm.cv_txt_.length());
#endif
#if VI_TM_STAT_USE_MINMAX
		max_len_min_ = std::max(max_len_min_, itm.min_txt_.length());
		max_len_max_ = std::max(max_len_max_, itm.max_txt_.length());
#endif
#if VI_TM_STAT_USE_BASE || VI_TM_STAT_USE_FILTER
		max_len_average_ = std::max(max_len_average_, itm.average_txt_.length());
		max_len_amount_ = std::max(max_len_amount_, itm.cnt_txt_.length());
#endif
	}
}

std::size_t formatter_t::width_column(vi_tmReportFlags_e clmn) const
{	std::size_t result = 0;
	std::size_t title_len = 0;
	switch (clmn)
	{	case vi_tmSortByName:
			result = max_len_name_;
			title_len = TitleName.length();
			break;
		case vi_tmSortBySpeed:
			result = max_len_average_;
			title_len = TitleAverage.length();
			break;
		case vi_tmSortByTime:
			result = max_len_total_;
			title_len = TitleTotal.length();
			break;
		case vi_tmSortByAmount:
			result = max_len_amount_;
			title_len = TitleAmount.length();
			break;
		default:
			assert(false);
			break;
	}

	if (to_sort_flag(flags_) == clmn)
	{	title_len += (flags_ & vi_tmSortAscending ? Ascending.length(): Descending.length());
	}
	result = std::max(title_len, result);

	return result;
}

std::string formatter_t::item_column(vi_tmReportFlags_e clmn) const
{	std::string result;
	switch (clmn)
	{
	case vi_tmSortByName:
		result += TitleName;
		break;
	case vi_tmSortBySpeed:
		result += TitleAverage;
		break;
	case vi_tmSortByTime:
		result += TitleTotal;
		break;
	case vi_tmSortByAmount:
		result += TitleAmount;
		break;
	default:
		assert(false);
		break;
	}

	if (to_sort_flag(flags_) == clmn)
	{	result += (flags_ & vi_tmSortAscending ? Ascending : Descending);
	}

	return result;
}

template<typename F>
int formatter_t::print_header(const F &fn) const
{	if (flags_ & vi_tmHideHeader)
	{	return 0;
	}

	std::ostringstream str;
	str <<
		std::setw(max_len_number_) << TitleNumber << "  " <<
		std::left << std::setfill(UNDERSCORE) <<
		std::setw(width_column(vi_tmSortByName)) << item_column(vi_tmSortByName) << ": " <<
		std::right << std::setfill(' ') <<
#if VI_TM_STAT_USE_BASE || VI_TM_STAT_USE_FILTER
		"" << std::setw(width_column(vi_tmSortBySpeed)) << item_column(vi_tmSortBySpeed) << " " <<
#endif
#if VI_TM_STAT_USE_FILTER
		"+/- " << std::setw(max_len_cv_) << TitleCV << " " <<
#endif
#if VI_TM_STAT_USE_BASE
		"~= " << std::setw(width_column(vi_tmSortByTime)) << item_column(vi_tmSortByTime) << " / " <<
		std::setw(width_column(vi_tmSortByAmount)) << item_column(vi_tmSortByAmount) << " " <<
#endif
#if VI_TM_STAT_USE_MINMAX
		"[" << std::setw(max_len_min_) << TitleMin << " - " << std::setw(max_len_max_) << TitleMax << "] " <<
#endif
		"\n";
	const std::size_t len = str.tellp();
	str << std::setfill('-') << std::setw(len - 1) << "\n";
	return fn(str.str().c_str());
}

template<typename F>
int formatter_t::print_metering(const metering_t &i, const F &fn) const
{	std::ostringstream str;
	str.imbue(std::locale(str.getloc(), new misc::space_out));

	n_++;
	const char fill = ((0U != guideline_interval_) &&
		(0U == n_ % static_cast<std::size_t>(guideline_interval_))) ? UNDERSCORE : ' ';

	str <<
		std::setw(max_len_number_) << n_ << ". " << 
		std::left << std::setfill(fill) <<
		std::setw(width_column(vi_tmSortByName)) << i.name_ << ": " <<
		std::right << std::setfill(' ') <<
#if VI_TM_STAT_USE_BASE || VI_TM_STAT_USE_FILTER
		"" << std::setw(width_column(vi_tmSortBySpeed)) << i.average_txt_ << " " <<
#endif
#if VI_TM_STAT_USE_FILTER
		(i.cv_txt_.empty()? "    ": "+/- ") << std::setw(max_len_cv_) << i.cv_txt_ << " " <<
#endif
#if VI_TM_STAT_USE_BASE
		"~= " << std::setw(width_column(vi_tmSortByTime)) << i.sum_txt_ << " / " <<
		std::setw(width_column(vi_tmSortByAmount)) << i.cnt_ << " " <<
#endif
#if VI_TM_STAT_USE_MINMAX
		"[" << std::setw(max_len_min_) << i.min_txt_ << " - " << std::setw(max_len_max_) << i.max_txt_ << "] " <<
#endif
		"\n";
	return fn(str.str().c_str());
}

int VI_TM_CALL vi_tmReport(VI_TM_HJOUR journal_handle, unsigned flags, vi_tmReportCb_t fn, void *ctx)
{	assert(!ctx || !!fn);
	if (nullptr == fn)
	{	return 0;
	}

	auto metering_entries = get_meterings(journal_handle, flags);
	std::sort(metering_entries.begin(), metering_entries.end(), comparator_t{ flags });
	const formatter_t formatter{ metering_entries, flags };
	const auto prn = [fn, ctx](const char *str) { return fn(str, ctx); };

	int result = print_props(prn, flags);
	result += formatter.print_header(prn);
	for (const auto &itm : metering_entries)
	{	result += formatter.print_metering(itm, prn);
	}

	return result;
}

int VI_SYS_CALL vi_tmReportCb(const char *str, void* stream)
{	assert(nullptr == stream); // The output stream must be from the same RTL library as the output function!
	(void)stream;
#ifdef _WIN32
	if (auto h = ::GetStdHandle(STD_OUTPUT_HANDLE); nullptr == h || INVALID_HANDLE_VALUE == h) // In /SUBSYSTEM:WINDOWS, stdout does not work by default.
	{	::OutputDebugStringA(str); // Output to the debug console.
		return 0;
	}
#endif
	const auto result = fputs(str, stdout); // stdout!!!
	assert(result >= 0);
	return result;
}
