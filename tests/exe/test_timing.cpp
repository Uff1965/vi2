// This is an independent project of an individual developer. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: http://www.viva64.com

#include <vi_timing/vi_timing.hpp>

#include <gtest/gtest.h>

#include <cassert>
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
		const auto m = vi_tmMeasurement(journal.get(), NAME);

		{
			static constexpr auto samples_simple =
			{ 10010U, 9981U, 9948U, 10030U, 10053U, 9929U, 9894U
			};

			for (auto x : samples_simple)
			{
				vi_tmMeasurementAdd(m, x);
			}

			const char *name = nullptr;
			vi_tmMeasurementStats_t md;
			vi_tmMeasurementStatsReset(&md);
			vi_tmMeasurementGet(m, &name, &md);
			{
				EXPECT_TRUE(name);
				EXPECT_EQ(std::strcmp(name, NAME), 0);
				EXPECT_EQ(md.calls_, std::size(samples_simple));
#if VI_TM_STAT_USE_RAW
				EXPECT_EQ(md.cnt_, md.calls_);
				EXPECT_EQ(md.sum_, std::accumulate(std::begin(samples_simple), std::end(samples_simple), 0U));
#endif
#if VI_TM_STAT_USE_FILTER
				EXPECT_EQ(md.flt_cnt_, static_cast<VI_TM_FP>(md.calls_));
				EXPECT_EQ(md.flt_calls_, md.calls_);
#endif
#if VI_TM_STAT_USE_MINMAX
				EXPECT_EQ(md.min_, *std::min_element(std::begin(samples_simple), std::end(samples_simple)));
				EXPECT_EQ(md.max_, *std::max_element(std::begin(samples_simple), std::end(samples_simple)));
#endif
			}

#if VI_TM_STAT_USE_FILTER
			vi_tmMeasurementAdd(m, 10111); // It should be filtered out.
			{
				vi_tmMeasurementStats_t tmp;
				vi_tmMeasurementStatsReset(&tmp);

				vi_tmMeasurementGet(m, &name, &tmp);

				EXPECT_EQ(tmp.calls_, md.calls_ + 1U);
				EXPECT_EQ(tmp.calls_, md.flt_calls_ + 1U);
			}
			vi_tmMeasurementGet(m, &name, &md);

			vi_tmMeasurementAdd(m, 10110); // It should not be filtered out.
			{
				vi_tmMeasurementStats_t tmp;
				vi_tmMeasurementStatsReset(&tmp);

				vi_tmMeasurementGet(m, &name, &tmp);

				EXPECT_EQ(tmp.calls_, md.calls_ + 1U);
				EXPECT_EQ(tmp.flt_calls_, md.flt_calls_ + 1U);
				EXPECT_EQ(tmp.flt_cnt_, md.flt_cnt_ + fp_ONE);
			}
			vi_tmMeasurementGet(m, &name, &md);
#endif
		}
	};

	TEST(main, Stats)
	{
		static constexpr auto samples_simple =
		{	10010U, 9981U, 9948U, 10030U, 10053U,
			9929U, 9894U, 10110U, 10040U, 10110U,
			10019U, 9961U, 10078U, 9959U, 9966U,
			10030U, 10089U, 9908U, 9938U, 9890U,
		};
		static constexpr auto M = 2;
		static constexpr auto samples_multiple = { 990U, }; // Samples that will be added M times at once.
		static constexpr auto samples_exclude = { 200000U }; // Samples that will be excluded from the statistics.

		static constexpr auto exp_flt_cnt = std::size(samples_simple) + M * std::size(samples_multiple); // The total number of samples that will be counted.
		static const auto exp_flt_mean =
			(std::accumulate(std::cbegin(samples_simple), std::cend(samples_simple), 0.0) +
				static_cast<double>(M) * std::accumulate(std::cbegin(samples_multiple), std::cend(samples_multiple), 0.0)
			) / static_cast<VI_TM_FP>(exp_flt_cnt); // The mean value of the samples that will be counted.
		static constexpr char NAME[] = "dummy"; // The name of the measurement.

		vi_tmMeasurementStats_t md; // Measurement data to be filled in.
		vi_tmMeasurementStatsReset(&md);

		unique_journal_t journal{ vi_tmJournalCreate(), vi_tmJournalClose }; // Journal for measurements, automatically closed on destruction.
		{
			const auto m = vi_tmMeasurement(journal.get(), NAME); // Create a measurement 'NAME'.
			for (auto x : samples_simple) // Add simple samples one at a time.
			{	vi_tmMeasurementAdd(m, x);
			}

			for (auto x : samples_multiple) // Add multiple samples M times at once.
			{	vi_tmMeasurementStatsAdd(&md, M * x, M);
			}

			vi_tmMeasurementMerge(m, &md); // Merge the statistics into the measurement.
			vi_tmMeasurementStatsReset(&md);

#if VI_TM_STAT_USE_FILTER
			for (auto x : samples_exclude) // Add samples that will be excluded from the statistics.
			{	vi_tmMeasurementAdd(m, x, 1);
			}
#endif
			const char *name = nullptr; // Name of the measurement to be filled in.
			vi_tmMeasurementGet(m, &name, &md); // Get the measurement data and name.
			EXPECT_STREQ(name, NAME);
		}

#if VI_TM_STAT_USE_FILTER
		EXPECT_EQ(md.calls_, std::size(samples_simple) + std::size(samples_multiple) + std::size(samples_exclude));
#	if VI_TM_STAT_USE_RAW
		EXPECT_EQ(md.cnt_, std::size(samples_simple) + M * std::size(samples_multiple) + std::size(samples_exclude));
#	endif
		EXPECT_EQ(md.flt_cnt_, static_cast<VI_TM_FP>(exp_flt_cnt));
		EXPECT_LT(std::abs(md.flt_avg_ - exp_flt_mean) / exp_flt_mean, fp_EPSILON);
		EXPECT_FLOAT_EQ((md.flt_avg_ - exp_flt_mean) / exp_flt_mean, fp_ZERO);
		const auto s = std::sqrt(md.flt_ss_ / (md.flt_cnt_));
		const auto exp_flt_stddev = [] // The standard deviation of the samples that will be counted.
			{
				const auto sum_squared_deviations =
					std::accumulate
					(	std::cbegin(samples_simple),
						std::cend(samples_simple),
						0.0,
						[](auto i, auto v) { const auto d = static_cast<VI_TM_FP>(v) - exp_flt_mean; return FMA(d, d, i); }
					) +
					static_cast<double>(M) * std::accumulate
					(	std::cbegin(samples_multiple),
						std::cend(samples_multiple),
						0.0,
						[](auto i, auto v) { const auto d = static_cast<VI_TM_FP>(v) - exp_flt_mean; return FMA(d, d, i); }
					);
				return std::sqrt(sum_squared_deviations / static_cast<VI_TM_FP>(exp_flt_cnt));
			}();
		EXPECT_LT(std::abs(s - exp_flt_stddev) / exp_flt_stddev, fp_EPSILON);
		EXPECT_FLOAT_EQ((s - exp_flt_stddev) / exp_flt_stddev, fp_ZERO);
#else
		EXPECT_EQ(md.calls_, std::size(samples_simple) + std::size(samples_multiple));
#	if VI_TM_STAT_USE_RAW
		EXPECT_EQ(md.cnt_, std::size(samples_simple) + M * std::size(samples_multiple));
		EXPECT_LT(std::abs(static_cast<VI_TM_FP>(md.sum_) / md.cnt_ - exp_flt_mean), fp_EPSILON);
		EXPECT_FLOAT_EQ((static_cast<VI_TM_FP>(md.sum_) / md.cnt_ - exp_flt_mean), fp_ZERO);
#	endif
#endif
	};
} // namespace
