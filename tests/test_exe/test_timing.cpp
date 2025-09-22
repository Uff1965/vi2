// This is an independent project of an individual developer. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: http://www.viva64.com

#include <vi_timing/vi_timing.hpp>

#include <gtest/gtest.h>

#include <algorithm>
#include <cassert>
#include <cmath> // std::sqrt, std::abs, std::fma
#include <numeric> // std::accumulate

namespace
{
	// Checking for FMA support by the compiler/platform
#if defined(__FMA__) || (defined(_MSC_VER) && (defined(__AVX2__) || defined(__AVX512F__)))
#	define FMA(x, y, z) std::fma((x), (y), (z))
#else
#	define FMA(x, y, z) ((x) * (y) + (z))
#endif

	using unique_journal_t = std::unique_ptr<std::remove_pointer_t<VI_TM_HJOUR>, decltype(&vi_tmJournalClose)>;

	using fp_limits_t = std::numeric_limits<VI_TM_FP>;
	constexpr auto fp_ZERO = static_cast<VI_TM_FP>(0);
	constexpr auto fp_ONE = static_cast<VI_TM_FP>(1);
	constexpr auto fp_EPSILON = fp_limits_t::epsilon();
	constexpr char NAME[] = "dummy";

	template<std::size_t NS, std::size_t NX, std::size_t NM>
	constexpr vi_tmMeasurementStats_t calc
	(	const VI_TM_TICK (&s)[NS],
		const VI_TM_TICK (&x)[NX],
		const VI_TM_TICK (&m)[NM],
		std::size_t M
	)
	{	(void)M;
		vi_tmMeasurementStats_t result;
		vi_tmStatsReset(&result);
		result.calls_ = NS + NX + NM;
		assert(result.calls_);
#if VI_TM_STAT_USE_RAW
		result.cnt_ = NS + NX + M * NM;
		result.sum_ =
			std::accumulate(std::begin(s), std::end(s), static_cast<VI_TM_TICK>(0U)) +
			std::accumulate(std::begin(x), std::end(x), static_cast<VI_TM_TICK>(0U)) +
			M * std::accumulate(std::begin(m), std::end(m), static_cast<VI_TM_TICK>(0U));
#endif

#if VI_TM_STAT_USE_RMSE
#	if VI_TM_STAT_USE_FILTER
		result.flt_calls_ = NS + NM;
		result.flt_cnt_ = static_cast<VI_TM_FP>(NS + M * NM);
		result.flt_avg_ = 
			(	std::accumulate(std::cbegin(s), std::cend(s), fp_ZERO) +
				M * std::accumulate(std::cbegin(m), std::cend(m), fp_ZERO)
			) / result.flt_cnt_;
		auto func = [mean = result.flt_avg_](auto i, VI_TM_TICK v)
			{	const VI_TM_FP d = static_cast<VI_TM_FP>(v) - mean;
				return FMA(d, d, i);
			};
		result.flt_ss_ = [func, s, x, m, M]
			{	return
					std::accumulate(std::cbegin(s), std::cbegin(s) + NS, fp_ZERO, func) +
					M * std::accumulate(std::cbegin(m), std::cbegin(m) + NM, fp_ZERO, func);
			}();
#	else
		result.flt_calls_ = NS + NX + NM;
		result.flt_cnt_ = static_cast<VI_TM_FP>(NS + NX + M * NM);
		result.flt_avg_ = 
			(	std::accumulate(std::cbegin(s), std::cend(s), fp_ZERO) +
				std::accumulate(std::cbegin(x), std::cend(x), fp_ZERO) +
				M * std::accumulate(std::cbegin(m), std::cend(m), fp_ZERO)
			) / result.flt_cnt_;
		result.flt_ss_ = [func, s, x, m, M]
			{	return
					std::accumulate(std::cbegin(s), std::cbegin(s) + NS, fp_ZERO, func) +
					std::accumulate(std::cbegin(x), std::cbegin(x) + NX, fp_ZERO, func) +
					M * std::accumulate(std::cbegin(m), std::cbegin(m) + NM, fp_ZERO, func);
			}();
#	endif
#endif

#if VI_TM_STAT_USE_MINMAX
		result.min_ = static_cast<VI_TM_FP>(std::min
		({ *std::min_element(std::begin(s), std::end(s)),
			*std::min_element(std::begin(x), std::end(x)),
			*std::min_element(std::begin(m), std::end(m))
		}));
		result.max_ = static_cast<VI_TM_FP>(std::max
		({ *std::max_element(std::begin(s), std::end(s)),
			*std::max_element(std::begin(x), std::end(x)),
			*std::max_element(std::begin(m), std::end(m))
		}));
#endif
		return result;
	}

	void expect_eq(const vi_tmMeasurementStats_t &l, const vi_tmMeasurementStats_t &r)
	{
		EXPECT_EQ(l.calls_, r.calls_);
#if VI_TM_STAT_USE_RAW
		EXPECT_EQ(l.cnt_, r.cnt_);
		EXPECT_EQ(l.sum_, r.sum_);
#endif
#if VI_TM_STAT_USE_RMSE
		EXPECT_DOUBLE_EQ(l.flt_avg_, r.flt_avg_);
		EXPECT_DOUBLE_EQ(l.flt_calls_, r.flt_calls_);
		EXPECT_DOUBLE_EQ(l.flt_cnt_, r.flt_cnt_);
		EXPECT_DOUBLE_EQ(l.flt_ss_, r.flt_ss_);
#endif
#if VI_TM_STAT_USE_MINMAX
		EXPECT_EQ(l.min_, r.min_);
		EXPECT_EQ(l.max_, r.max_);
#endif
	}

	constexpr VI_TM_TICK SAMPLES_SIMPLE[] =
	{	10010U, 9981U, 9948U, 10030U, 10053U,
		9929U, 9894U, 10110U, 10040U, 10110U,
		10019U, 9961U, 10078U, 9959U, 9966U,
		10030U, 10089U, 9908U, 9938U, 9890U,
	};
	constexpr VI_TM_TICK SAMPLES_EXCLUDE[] = { 200000U }; // Samples that will be excluded from the statistics.
	constexpr VI_TM_TICK SAMPLES_MULTIPLE[] = { 990U, }; // Samples that will be added M times at once.
	constexpr std::size_t M = 2;
	
	const auto EXPECTED = calc(SAMPLES_SIMPLE, SAMPLES_EXCLUDE, SAMPLES_MULTIPLE, M);

	TEST(main, vi_tmMeasurementStats)
	{
		const unique_journal_t journal{ vi_tmJournalCreate(), vi_tmJournalClose };
		const auto hmeas = vi_tmJournalGetMeas(journal.get(), NAME);

		{
			static constexpr auto samples_simple =
			{ 10010U, 9981U, 9948U, 10030U, 10053U, 9929U, 9894U
			};

			for (auto x : samples_simple)
			{	vi_tmMeasurementAdd(hmeas, x);
			}

			const char *name = nullptr;
			vi_tmMeasurementStats_t md;
			vi_tmStatsReset(&md);

			vi_tmMeasurementGet(hmeas, &name, &md);
			{	EXPECT_TRUE(name);
				EXPECT_STREQ(name, NAME);
				EXPECT_EQ(md.calls_, std::size(samples_simple));
#if VI_TM_STAT_USE_RAW
				EXPECT_EQ(md.cnt_, md.calls_);
				EXPECT_EQ(md.sum_, std::accumulate(std::begin(samples_simple), std::end(samples_simple), 0U));
#endif
#if VI_TM_STAT_USE_RMSE
				EXPECT_EQ(md.flt_cnt_, static_cast<VI_TM_FP>(md.calls_));
				EXPECT_EQ(md.flt_calls_, md.calls_);
#endif
#if VI_TM_STAT_USE_MINMAX
				EXPECT_EQ(md.min_, *std::min_element(std::begin(samples_simple), std::end(samples_simple)));
				EXPECT_EQ(md.max_, *std::max_element(std::begin(samples_simple), std::end(samples_simple)));
#endif
			}

#if VI_TM_STAT_USE_RMSE
#	if VI_TM_STAT_USE_FILTER
			vi_tmMeasurementAdd(hmeas, 10111); // It should be filtered out.
			{	vi_tmMeasurementStats_t tmp;
				vi_tmStatsReset(&tmp);

				vi_tmMeasurementGet(hmeas, &name, &tmp);

				EXPECT_EQ(tmp.calls_, md.calls_ + 1U);
				EXPECT_EQ(tmp.calls_, md.flt_calls_ + 1U);
			}
			vi_tmMeasurementGet(hmeas, &name, &md);
#	endif
			vi_tmMeasurementAdd(hmeas, 10110); // It should not be filtered out.
			{	vi_tmMeasurementStats_t tmp;
				vi_tmStatsReset(&tmp);

				vi_tmMeasurementGet(hmeas, &name, &tmp);

				EXPECT_EQ(tmp.calls_, md.calls_ + 1U);
				EXPECT_EQ(tmp.flt_calls_, md.flt_calls_ + 1U);
				EXPECT_EQ(tmp.flt_cnt_, md.flt_cnt_ + fp_ONE);
			}
#endif
			vi_tmMeasurementGet(hmeas, &name, &md);
		}
	};

	TEST(main, RMSE)
	{	unique_journal_t journal{ vi_tmJournalCreate(), vi_tmJournalClose }; // Journal for measurements, automatically closed on destruction.
		const auto hmeas = vi_tmJournalGetMeas(journal.get(), NAME); // Create a measurement 'NAME'.
		for (auto x : SAMPLES_SIMPLE) // Add simple samples one at a time.
		{	vi_tmMeasurementAdd(hmeas, x);
		}
		for (auto x : SAMPLES_EXCLUDE) // Add samples that will be excluded from the statistics.
		{	vi_tmMeasurementAdd(hmeas, x, 1);
		}
		for (auto x : SAMPLES_MULTIPLE) // Add multiple samples M times at once.
		{	vi_tmMeasurementAdd(hmeas, M * x, M);
		}

		const char *name = nullptr; // Name of the measurement to be filled in.
		vi_tmMeasurementStats_t md; // Measurement data to be filled in.
		vi_tmStatsReset(&md);
		vi_tmMeasurementGet(hmeas, &name, &md); // Get the measurement data and name.
		EXPECT_STREQ(name, NAME);

		expect_eq(EXPECTED, md);
	};

	TEST(main, Merge)
	{	unique_journal_t journal{ vi_tmJournalCreate(), vi_tmJournalClose }; // Journal for measurements, automatically closed on destruction.
		const auto hmeas = vi_tmJournalGetMeas(journal.get(), NAME); // Create a measurement 'NAME'.

		for (auto x : SAMPLES_SIMPLE) // Add simple samples one at a time.
		{	vi_tmMeasurementAdd(hmeas, x);
		}
		for (auto x : SAMPLES_EXCLUDE) // Add samples that will be excluded from the statistics.
		{	vi_tmMeasurementAdd(hmeas, x, 1);
		}

		vi_tmMeasurementStats_t meas;
		vi_tmStatsReset(&meas);
		for (auto x : SAMPLES_MULTIPLE) // Add multiple samples M times at once.
		{	vi_tmStatsAdd(&meas, M * x, M);
		}
		EXPECT_EQ(vi_tmStatsIsValid(&meas), 0);

		vi_tmMeasurementMerge(hmeas, &meas);
		vi_tmStatsReset(&meas);
		vi_tmMeasurementGet(hmeas, nullptr, &meas);
		EXPECT_EQ(vi_tmStatsIsValid(&meas), 0);
		expect_eq(EXPECTED, meas);
	}
} // namespace
