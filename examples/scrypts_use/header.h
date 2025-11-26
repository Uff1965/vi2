#pragma once
#include <string>

#define FILE_PATH (__FILE__ + sizeof(VI_TM_ROOT_DIR))

using test_func_t = void (*)(void);
int register_test(test_func_t fn);
