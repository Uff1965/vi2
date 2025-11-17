#include <vi_timing/vi_timing.hpp>

#if VI_TM_ENABLE_BENCHMARK
#	include <benchmark/benchmark.h>
#endif

#include <algorithm>
#include <cmath>
#include <charconv>
#include <cstdio>
#include <format>
#include <iomanip>
#include <iostream>
#include <random>
#include <sstream>
#include <string>

namespace
{
	std::size_t dummy(char *, std::size_t, double)
	{	return 0;
	}

	std::size_t std_snprintf(char *buff, std::size_t sz, double f)
	{	return std::snprintf(buff, sz, "%3.2f", f);
	}

	std::size_t std_format(char *buff, std::size_t sz, double f)
	{	auto res = std::format_to_n(buff, sz - 1, "{:3.2f}", f);
		*res.out = '\0';
		return res.size;
	}

	std::size_t std_to_chars(char *buff, std::size_t sz, double f)
	{	std::to_chars_result r = std::to_chars(buff, buff + sz, f, std::chars_format::fixed, 2);
		*r.ptr = '\0';
		return r.ptr - buff;
	}

	std::size_t std_ostringstream(char *buff, std::size_t sz, double f)
	{	const auto str = (std::ostringstream{} << std::fixed << std::setprecision(2) << std::setw(3) << f).str();
		std::strncpy(buff, str.data(), sz);
		return str.length();
	}

	std::size_t std_to_string(char *buff, std::size_t sz, double f)
	{	std::string str = std::to_string(f);
		std::strncpy(buff, str.data(), sz);
		return str.length();
	}

	struct func_desc_t
	{	std::string name_;
		decltype(&dummy) func_;

		// defaulted three-way is OK: compares name_ then pointer
		auto operator<=>(const func_desc_t &r) const { return name_ <=> r.name_; };
	};

	void fill_random_doubles(double *buff, std::size_t cnt)
	{	std::mt19937_64 gen/*(std::random_device{}())*/;
		std::uniform_real_distribution<double> dist(0.0, 1e3);
		for (auto n = cnt; n; )
		{	if (auto f = dist(gen); std::isnormal(f))
			{	--n;
				buff[n] = f;
			}
		}
	}

	VI_TM("Global");
#ifdef NDEBUG
	constexpr auto CNT = 1'000U;
#else
	constexpr auto CNT = 200U;
#endif
	double arr[CNT];
	char buff[64];

} // namespace

#if VI_TM_ENABLE_BENCHMARK
#	define BM_DNO(n) benchmark::DoNotOptimize(n); benchmark::ClobberMemory()

#	define BM_FN(name)\
	static void BM_ ## name(benchmark::State &state)\
	{	std::size_t n = 0;\
		for (auto _ : state)\
		{	auto tmp = name(buff, std::size(buff), arr[n % std::size(arr)]);\
			BM_DNO(tmp);\
			++n;\
		}\
	}\
	BENCHMARK(BM_ ## name)

	BM_FN(std_ostringstream);
	BM_FN(std_to_string);
	BM_FN(std_snprintf);
	BM_FN(std_format);
	BM_FN(std_to_chars);
	BM_FN(dummy);
#else
#	define BM_DNO(n)
#endif

int main(int argc, char** argv)
{	vi_tmInit("Timing report:\n", vi_tmShowResolution | vi_tmShowDuration | vi_tmShowOverhead | vi_tmSortBySpeed);
	vi_CurrentThreadAffinityFixate();
	vi_WarmUp(1);

	fill_random_doubles(arr, std::size(arr));

#if VI_TM_ENABLE_BENCHMARK
	std::cout << "Benchmark:" << std::endl;
	::benchmark::Initialize(&argc, argv);
	if (::benchmark::ReportUnrecognizedArguments(argc, argv))
	{	return 1;
	}
	::benchmark::RunSpecifiedBenchmarks();
	std::cout << "Benchmark - done." << std::endl;
#endif

	func_desc_t fncs[] = {
		{ "_dummy", dummy },
		{ "format", std_format },
		{ "ostringstream", std_ostringstream },
		{ "snprintf", std_snprintf },
		{ "to_chars", std_to_chars },
		{ "to_string", std_to_string },
	};
	
	{	VI_TM("Amortized Time");
		std::cout << "Amortized Time..." << std::flush;
		for (auto const &fn : fncs)
		{	vi_ThreadYield();
			constexpr auto SIZE = 720 * std::size(arr);
			VI_TM((fn.name_ + "_agr").c_str(), SIZE);
			for (auto n = SIZE; n--; )
			{	auto tmp = fn.func_(buff, std::size(buff), arr[n % std::size(arr)]);
				BM_DNO(tmp);
			}
		}
		std::cout << " done." << std::endl;
	}

	{	VI_TM("Per-Operation Mean");
		std::cout << "Per-Operation Mean..." << std::flush;
		for (auto n = std::size(arr); n--; )
		{	do
			{	vi_ThreadYield();
				for (auto const &fn : fncs)
				{	VI_TM(fn.name_.c_str());
					auto tmp = fn.func_(buff, std::size(buff), arr[n % std::size(arr)]);
					BM_DNO(tmp);
				}
			} while (std::next_permutation(std::begin(fncs), std::end(fncs)));
		}
		std::cout << " done." << std::endl;
	}

	vi_CurrentThreadAffinityRestore();
}
