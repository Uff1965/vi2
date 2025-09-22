// This is an independent project of an individual developer. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: http://www.viva64.com

#include <vi_timing/vi_timing.hpp>

#include <gtest/gtest.h>

#include <cassert>
#include <cmath> // std::sqrt, std::abs, std::fma
#include <numeric> // std::accumulate

namespace
{
	// Checking for FMA support by the compiler/platform
#if defined(__FMA__) || (defined(_MSC_VER) && (defined(__AVX2__) || defined(__AVX512F__)))
#define FMA(x, y, z) std::fma((x), (y), (z))
#else
#define FMA(x, y, z) ((x) * (y) + (z))
#endif

	using unique_journal_t = std::unique_ptr<std::remove_pointer_t<VI_TM_HJOUR>, decltype(&vi_tmJournalClose)>;

	using fp_limits_t = std::numeric_limits<VI_TM_FP>;
	constexpr auto fp_ZERO = static_cast<VI_TM_FP>(0);
	constexpr auto fp_ONE = static_cast<VI_TM_FP>(1);
	constexpr auto fp_EPSILON = fp_limits_t::epsilon();

	TEST(main, vi_tmMeasurementStats)
	{
		static constexpr char NAME[] = "dummy";
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
				EXPECT_EQ(std::strcmp(name, NAME), 0);
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

	TEST(main, Stats)
	{
		static constexpr auto SAMPLES_SIMPLE =
		{	10010U, 9981U, 9948U, 10030U, 10053U,
			9929U, 9894U, 10110U, 10040U, 10110U,
			10019U, 9961U, 10078U, 9959U, 9966U,
			10030U, 10089U, 9908U, 9938U, 9890U,
		};
		static constexpr auto M = 2;
		static constexpr auto SAMPLES_MULTIPLE = { 990U, }; // Samples that will be added M times at once.
		static constexpr auto SAMPLES_EXCLUDE = { 200000U }; // Samples that will be excluded from the statistics.
		static constexpr auto EXP_CALLS =
			std::size(SAMPLES_SIMPLE) +
			std::size(SAMPLES_EXCLUDE) +
			std::size(SAMPLES_MULTIPLE);
		static constexpr auto EXP_CNT = // The total number of samples that will be counted.
			std::size(SAMPLES_SIMPLE) +
			std::size(SAMPLES_EXCLUDE) +
			M * std::size(SAMPLES_MULTIPLE);
		static constexpr auto EXP_FLT_CALLS =
			std::size(SAMPLES_SIMPLE) +
#if !VI_TM_STAT_USE_FILTER
			std::size(SAMPLES_EXCLUDE) +
#endif
			std::size(SAMPLES_MULTIPLE);
		static constexpr auto EXP_FLT_CNT = // The total number of samples that will be filtered counted.
			std::size(SAMPLES_SIMPLE) +
#if !VI_TM_STAT_USE_FILTER
			std::size(SAMPLES_EXCLUDE) +
#endif
			M * std::size(SAMPLES_MULTIPLE);
		static const auto EXP_FLT_MEAN =
			(	std::accumulate(std::cbegin(SAMPLES_SIMPLE), std::cend(SAMPLES_SIMPLE), 0.0) +
#if !VI_TM_STAT_USE_FILTER
				std::accumulate(std::cbegin(SAMPLES_EXCLUDE), std::cend(SAMPLES_EXCLUDE), 0.0) +
#endif
				static_cast<double>(M) * std::accumulate(std::cbegin(SAMPLES_MULTIPLE), std::cend(SAMPLES_MULTIPLE), 0.0)
			) / static_cast<VI_TM_FP>(EXP_FLT_CNT); // The mean value of the samples that will be counted.
		static constexpr char NAME[] = "dummy"; // The name of the measurement.

		vi_tmMeasurementStats_t md; // Measurement data to be filled in.
		vi_tmStatsReset(&md);

		unique_journal_t journal{ vi_tmJournalCreate(), vi_tmJournalClose }; // Journal for measurements, automatically closed on destruction.
		{
			const auto hmeas = vi_tmJournalGetMeas(journal.get(), NAME); // Create a measurement 'NAME'.
			for (auto x : SAMPLES_SIMPLE) // Add simple samples one at a time.
			{	vi_tmMeasurementAdd(hmeas, x);
			}
			for (auto x : SAMPLES_EXCLUDE) // Add samples that will be excluded from the statistics.
			{	vi_tmMeasurementAdd(hmeas, x, 1);
			}
			{	for (auto x : SAMPLES_MULTIPLE) // Add multiple samples M times at once.
				{	vi_tmStatsAdd(&md, M * x, M);
				}
				vi_tmMeasurementMerge(hmeas, &md); // Merge the statistics into the measurement.
			}
			vi_tmStatsReset(&md);
			const char *name = nullptr; // Name of the measurement to be filled in.
			vi_tmMeasurementGet(hmeas, &name, &md); // Get the measurement data and name.
			EXPECT_STREQ(name, NAME);
		}

		EXPECT_EQ(md.calls_, EXP_CALLS);
#if VI_TM_STAT_USE_RAW
		EXPECT_EQ(md.cnt_, EXP_CNT);
#endif

#if VI_TM_STAT_USE_RMSE
		EXPECT_EQ(md.flt_calls_, EXP_FLT_CALLS);
		EXPECT_EQ(md.flt_cnt_, static_cast<VI_TM_FP>(EXP_FLT_CNT));
		EXPECT_LT(std::abs(md.flt_avg_ - EXP_FLT_MEAN) / EXP_FLT_MEAN, fp_EPSILON);
		const auto s = std::sqrt(md.flt_ss_ / (md.flt_cnt_));
		const auto exp_flt_stddev = [] // The standard deviation of the samples that will be counted.
			{
				const auto sum_squared_deviations =
					std::accumulate
					(	std::cbegin(samples_simple),
						std::cend(samples_simple),
						0.0,
						[](auto i, auto v) { const auto d = static_cast<VI_TM_FP>(v) - EXP_FLT_MEAN; return FMA(d, d, i); }
					) +
#if !VI_TM_STAT_USE_FILTER
					std::accumulate
					(	std::cbegin(samples_exclude),
						std::cend(samples_exclude),
						0.0,
						[](auto i, auto v) { const auto d = static_cast<VI_TM_FP>(v) - EXP_FLT_MEAN; return FMA(d, d, i); }
					) +
#endif
					static_cast<double>(M) * std::accumulate
					(	std::cbegin(samples_multiple),
						std::cend(samples_multiple),
						0.0,
						[](auto i, auto v) { const auto d = static_cast<VI_TM_FP>(v) - EXP_FLT_MEAN; return FMA(d, d, i); }
					);
				return std::sqrt(sum_squared_deviations / static_cast<VI_TM_FP>(EXP_FLT_CNT));
			}();
		EXPECT_LT(std::abs(s - exp_flt_stddev) / exp_flt_stddev, fp_EPSILON);
#endif
	};
} // namespace
