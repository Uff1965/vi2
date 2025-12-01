#pragma once

#include <vi_timing/vi_timing_proxy.h>
#include <string>

#define FILE_PATH (__FILE__ + sizeof(VI_TM_ROOT_DIR))

using test_func_t = bool (*)(void);
int register_test(const char* name, test_func_t fn);

extern VI_TM_HREG g_register;
constexpr char MSG[] = "Hello, World!";
constexpr int KEY = 777;
constexpr int FIB_N = 20;
constexpr int FIB_R = 6'765;

#define TM(...) VI_TM_H(g_register, __VA_ARGS__)
