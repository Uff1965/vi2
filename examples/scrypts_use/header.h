#pragma once
#include <string>

using test_func_t = void (*)();
void register_test(std::string test_name, test_func_t*);
