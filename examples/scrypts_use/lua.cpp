#include "header.h"
#include <vi_timing/vi_timing.h>

extern "C" {
	#include <lua/lua.h>
	#include <lua/lualib.h>
	#include <lua/lauxlib.h>
}

namespace lua
{
	// C++ function exposed to Lua
	// This simulates a native callback that Lua can invoke.
	static int cpp_callback(lua_State *L)
	{	VI_TM("0: Lua callback");
		luaL_checkstring(L, 1);   // Validate first argument is a string
		lua_pushinteger(L, 42);   // Push a dummy integer result
		return 1;                 // Return one result to Lua
	}

	// Step 1: Initialize Lua state and open standard libraries
	lua_State* init()
	{   VI_TM("1: Lua Initialize");
		lua_State *L = luaL_newstate();   // Create new Lua interpreter state
		luaL_openlibs(L);                 // Load all standard Lua libraries
		lua_register(L, "cpp_callback", cpp_callback); // Register native function
		return L;
	}

	// Step 2: Load and compile Lua script
	bool load_script(lua_State *L)
	{   VI_TM("2: Lua Load and compile");
		static constexpr char lua_script[] =
			R"(function lua_worker()
				local result = cpp_callback('Hello from Lua!')
				end
			)";

		// Compile and execute script
		if (luaL_dostring(L, lua_script) != LUA_OK)
		{   fprintf(stderr, "Lua error: %s\n", lua_tostring(L, -1));
			return false;
		}
		return true;
	}

	// Step 3: Call Lua function
	bool call_worker(lua_State *L)
	{	VI_TM("3: Lua Call");
		lua_getglobal(L, "lua_worker");   // Push function onto stack

		if (!lua_isfunction(L, -1))
		{	fprintf(stderr, "Lua error: lua_worker not found\n");
			return false;
		}

		// Execute Lua function with no args and no return values
		if (lua_pcall(L, 0, 0, 0) != LUA_OK)
		{	fprintf(stderr, "Lua error: %s\n", lua_tostring(L, -1));
			return false;
		}
		return true;
	}

	// Step 4: Cleanup Lua state
	void cleanup(lua_State *L)
	{	VI_TM("4: Lua Cleanup");
		lua_close(L);   // Free all resources
	}

	// Test entry
	void test()
	{	VI_TM_FUNC;
		
		if (auto L = init())
		{	if (load_script(L))
			{	call_worker(L);
			}

			cleanup(L);
		}
	}

	const auto _ = register_test(test);

} // namespace lua
