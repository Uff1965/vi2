#include <vi_timing/vi_timing.hpp>

#include <string>

unsigned fib(unsigned n)
{	return n < 2 ? n : (fib(n - 1) + fib(n - 2));
};

constexpr int CNT = 3;
volatile unsigned N = 30;

VI_TM("Global scope");

static const auto _ = VI_TM_GLOBALINIT(
	vi_tmReportDefault,
	"Timing report:\n",
	"Success - the test program completed!\n"
);

int main()
{	VI_TM_FUNC;

	for (volatile int n = 0; n < CNT; n = n + 1)
	{	VI_TM(std::to_string(n).c_str());
		volatile auto _ = fib(N);
	}

	VI_TM("Fib ext", CNT);
	for (volatile int n = 0; n < CNT; n = n + 1)
	{	VI_TM_S("Fib int");
		volatile auto _ = fib(N);
	}
}
