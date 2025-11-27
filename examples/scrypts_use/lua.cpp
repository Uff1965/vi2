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
	{	TM("0: Lua callback");
		luaL_checkstring(L, 1);   // Validate first argument is a string
		lua_pushinteger(L, 42);   // Push a dummy integer result
		return 1;                 // Return one result to Lua
	}

	// Step 1: Initialize Lua state and open standard libraries
	lua_State* init()
	{   TM("1: Lua Initialize");
		lua_State *L = luaL_newstate();   // Create new Lua interpreter state
		luaL_openlibs(L);                 // Load all standard Lua libraries
		lua_register(L, "cpp_callback", cpp_callback); // Register native function
		return L;
	}

	// Step 2: Load and compile Lua script
	bool load_script(lua_State *L)
	{   TM("2: Lua Load and compile");
		static constexpr char script[] =
			R"(function lua_worker()
				local result = cpp_callback('Hello from Lua!')
				end
			)";

		//// Compile and execute script
		//if (luaL_dostring(L, script) != LUA_OK)
		//{   fprintf(stderr, "Lua error: %s\n", lua_tostring(L, -1));
		//	return false;
		//}

		{
			int status = LUA_OK;
			{	TM("2.1: Lua load");
				status = luaL_loadstring(L, script);
			}

			if (status == LUA_OK)
			{	TM("2.2: Lua call");
				status = lua_pcall(L, 0, LUA_MULTRET, 0);
			}

			if (status != LUA_OK)
			{	fprintf(stderr, "Lua error: %s\n", lua_tostring(L, -1));
				return false;
			}
		}

		return true;
	}

	// Step 3: Call Lua function
	bool call_worker(lua_State *L)
	{	lua_getglobal(L, "lua_worker");   // Push function onto stack

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

	bool call(lua_State *L)
	{
		{	TM("3.1: Lua First Call");
			if(!call_worker(L))
			{	return false;
			}
		}

		for(int n = 0; n < 100; ++n)
		{	TM("3.2: Lua Other Call");
			if(!call_worker(L))
			{	return false;
			}
		}

		return true;
	}

	// Step 4: Cleanup Lua state
	void cleanup(lua_State *L)
	{	TM("4: Lua Cleanup");
		lua_close(L);   // Free all resources
	}

	// Test entry
	void test()
	{	TM("lua test");
		
		if (auto L = init())
		{	if (load_script(L))
			{	call(L);
			}

			cleanup(L);
		}
	}

	const auto _ = register_test(test);

} // namespace lua
