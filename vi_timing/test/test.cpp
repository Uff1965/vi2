// This is an independent project of an individual developer. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: http://www.viva64.com

#if defined(_MSC_VER) && defined(_DEBUG)
#   define _CRTDBG_MAP_ALLOC //-V2573
#   include <crtdbg.h>
#   include <stdlib.h>
#endif

#include "header.h"

#include "../timing.h"

#include <iomanip>
#include <iostream>
#include <chrono>
#include <thread>
#include <vector>
#include <atomic>

#if defined(_MSC_VER) && defined(_DEBUG)
const auto _dummy0 = _CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF); // To automatically call the _CrtDumpMemoryLeaks function when the program ends
const auto _dummy1 = _set_error_mode(_OUT_TO_MSGBOX); // Error sink is a message box. To be able to ignore errors.
#endif

VI_TM_INIT("Timing report:", reinterpret_cast<vi_tmLogSTR_t>(&std::fputs), stdout, 0xE2);
VI_TM("GLOBAL");

namespace {
	using namespace std::literals;
	namespace ch = std::chrono;

	void warming(bool all, ch::milliseconds ms)
	{
		if (0 != ms.count())
		{
			std::atomic_bool done = false; // It must be defined before 'threads'!!!
			auto load = [&done] {while (!done) {/**/}}; //-V776

			const auto hwcnt = std::thread::hardware_concurrency();
			std::vector<std::thread> threads((all && hwcnt > 1) ? hwcnt - 1 : 0);
			for (auto& t : threads) { t = std::thread{ load }; }
			for (const auto stop = ch::steady_clock::now() + ms; ch::steady_clock::now() < stop;) {/*The thread is fully loaded.*/ }
			done = true;
			for (auto& t : threads) { t.join(); }
		}
	}

	bool test_multithreaded()
	{
		VI_TM_FUNC;

		static constexpr auto CNT = 100'000;
		static std::atomic<std::size_t> v{};

		std::cout << "\ntest_multithreaded()... " << std::endl;

		auto load = [](std::size_t n)
		{	VI_TM("load");

			for (auto m = CNT; m; --m)
			{	VI_TM(("thread_"s + std::to_string(n)).c_str());
				v++;
			}
		};

		std::vector<std::thread> threads{std::thread::hardware_concurrency() + 8};
		for (std::size_t n = 0; n < threads.size(); ++n)
		{	threads[n] = std::thread{load, n};
		}

		for (auto& t : threads)
		{	t.join();
		}

		std::cout << "v: " << v << std::endl;
		std::cout << "done" << std::endl;
		return true;
	}
}

bool test_misc()
{
	std::cout << "\ntest_quant()... " << std::endl;

	{
		std::this_thread::yield();
		vi_tmGetTicks();
		const auto s = vi_tmGetTicks();
		const auto f = vi_tmGetTicks();
		std::cout << "s: " << s << "\n";
		std::cout << "f: " << f << "\n";
		std::cout << "f - s: " << f - s << "\n";
		std::cout << "sizeof(vi_tmTicks_t): " << sizeof(vi_tmTicks_t) << " byte" << "\n";
		endl(std::cout);
	}

	{
		std::this_thread::yield();
		VI_TM("sleep_for 1s");
		std::this_thread::sleep_for(1s);
	}

	{
		std::this_thread::yield();
		VI_TM("sleep_for 30ms");
		std::this_thread::sleep_for(30ms);
	}

	{
		std::this_thread::yield();
		VI_TM("sleep_for 20ms");
		std::this_thread::sleep_for(20ms);
	}

	{
		std::this_thread::yield();
		VI_TM("sleep_for 14ms");
		std::this_thread::sleep_for(14ms);
	}

	{
		std::this_thread::yield();
		VI_TM("sleep_for 1ms");
		std::this_thread::sleep_for(1ms);
	}

	{
		std::this_thread::yield();
		VI_TM("sleep_for 100us");
		std::this_thread::sleep_for(100us);
	}

	{
		std::this_thread::yield();
		VI_TM("sleep_for 10us");
		std::this_thread::sleep_for(10us);
	}

	return true;
}

int main()
{
	struct space_out : std::numpunct<char> {
		char do_thousands_sep() const override { return '\''; }  // separate with spaces
		std::string do_grouping() const override { return "\3"; } // groups of 1 digit
	};
	std::cout.imbue(std::locale(std::cout.getloc(), new space_out));

	const std::time_t tm = std::chrono::system_clock::to_time_t(ch::system_clock::now());
#pragma warning(suppress: 4996)
	std::cout << "Start: " << std::put_time(std::localtime(&tm), "%F %T.\n");

	std::cout << "Build type: ";
#ifdef NDEBUG
	std::cout << "Release";
#else
	std::cout << "Debug";
#endif
	endl(std::cout);

	std::cout << "\nWarming... ";
	warming(true, 500ms);
	std::cout << "done";
	endl(std::cout);

	C_function();
	test_multithreaded();
	test_misc();

	std::cout << "\nHello, World!\n";
	endl(std::cout);
}
