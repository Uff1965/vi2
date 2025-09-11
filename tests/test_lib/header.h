#ifndef VI2_TESTS_LIB_HEADER_H
#	define VI2_TESTS_LIB_HEADER_H 0.1.0
#	pragma once

#ifdef VI_TM_TEST_LIB_EXPORTS
#		define VI_TM_TEST_LIB_API __declspec(dllexport)
#else
#		define VI_TM_TEST_LIB_API __declspec(dllimport)
#endif

extern "C" VI_TM_TEST_LIB_API void test_lib_func(void);

#endif // #ifndef VI2_TESTS_LIB_HEADER_H
