#ifndef VI2_TESTS_LIB_HEADER_H
#	define VI2_TESTS_LIB_HEADER_H 0.1.0
#	pragma once


#if defined(_MSC_VER)
#	if defined(VI_TM_TEST_LIB_EXPORTS)
#		define VI_TM_TEST_LIB_API __declspec(dllexport)
#	else
#		define VI_TM_TEST_LIB_API __declspec(dllimport)
#	endif
#elif defined (__GNUC__) || defined(__clang__)
#	ifdef VI_TM_TEST_LIB_EXPORTS
#		define VI_TM_TEST_LIB_API __attribute__((visibility("default")))
#	else
#		define VI_TM_TEST_LIB_API
#	endif
#else
#	define VI_TM_TEST_LIB_API
#endif

extern "C" VI_TM_TEST_LIB_API void test_shared_lib_func(void);

#endif // #ifndef VI2_TESTS_LIB_HEADER_H
