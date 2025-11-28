#pragma once

#include <vi_timing/vi_timing_proxy.h>
#include <string>

#define FILE_PATH (__FILE__ + sizeof(VI_TM_ROOT_DIR))

using test_func_t = void (*)(void);
int register_test(test_func_t fn);

extern VI_TM_HREG h_register;

#define TM(...) VI_TM_H(h_register, __VA_ARGS__)
