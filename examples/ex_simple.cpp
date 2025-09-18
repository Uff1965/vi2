#include <vi_timing/vi_timing.hpp>

#include <string>

unsigned fib(unsigned n)
{	return n < 2 ? n : (fib(n - 1) + fib(n - 2));
};

constexpr int CNT = 100;
volatile unsigned N = 37;

VI_TM("Global scope");

int main()
{	VI_TM_FUNC;

	for (volatile int n = 0; n < CNT; ++n)
	{	VI_TM_S(std::to_string(n).c_str());
		volatile auto _ = fib(N);
	}

	VI_TM("Fib ext", CNT);
	for (volatile int n = 0; n < CNT; ++n)
	{	VI_TM_S("Fib int");
		volatile auto _ = fib(N);
	}
}
