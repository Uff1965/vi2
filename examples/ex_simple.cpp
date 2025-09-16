#include <vi_timing/vi_timing.hpp>

#include <iostream>

unsigned fib(unsigned n)
{	return n < 2 ? n : (fib(n - 1) + fib(n - 2));
};

VI_TM("Global");

int main()
{	VI_TM_FUNC;

	{	VI_TM("Stream out");
		std::cout << "Hello, World!" << std::endl;
	}

	{	constexpr unsigned CNT = 1'000;
		volatile unsigned n = 32;
		volatile unsigned x = 0;

		VI_TM("Fibonacci all", CNT);
		for(unsigned i = 0; i < CNT; i++)
		{	VI_TM("Fibonacci");
			x = fib(n);
		}
	}

	{	constexpr unsigned CNT = 10'000'000;

		VI_TM("Empty all", CNT);
		for (auto i = 0U; i < CNT; i++)
		{	VI_TM("Empty");
		}
	}
}
