#include "header.h"

extern "C" {
	#include <lua/lua.h>
	#include <lua/lualib.h>
	#include <lua/lauxlib.h>
}

#include <cstdio>

namespace lua
{
	constexpr int VAL = 42;

	// C++ function exposed to Lua
	// This simulates a native callback that Lua can invoke.
	int cpp_callback(lua_State *L)
	{	TM("0: Lua callback");
		// Validate first argument is a string
		if (const char* message = luaL_checkstring(L, 1); !message || strcmp(message, "Hello, World!") != 0)
		{	assert(false);
			printf("Lua callback: unexpected string '%s'\n", message);
		}
		lua_pushinteger(L, VAL); // Push a dummy integer result
		return 1; // Return one result to Lua
	}

	// Step 1: Initialize Lua state and open standard libraries
	lua_State* init()
	{   TM("1: Lua Initialize");
		lua_State *L = luaL_newstate(); // Create new Lua interpreter state
		if (!L)
		{	assert(false);
			fprintf(stderr, "Lua error: failed to create state\n");
			return nullptr;
		}
		luaL_openlibs(L); // Load all standard Lua libraries
		lua_register(L, "cpp_callback", cpp_callback); // Register native function
		return L;
	}

	// Step 2: Load and compile Lua script
	bool load_script(lua_State *L)
	{   TM("2: Lua Load and compile");
		static constexpr char script[] = R"(
				function lua_worker()
					return cpp_callback('Hello, World!')
				end
			)";

		// Compile and execute script
		if (luaL_dostring(L, script) != LUA_OK)
		{	assert(false);
			fprintf(stderr, "Lua error: %s\n", lua_tostring(L, -1));
			lua_pop(L, 1);
			return false;
		}

		return true;
	}

	// Step 3: Call Lua function
	bool call_worker(lua_State *L)
	{	lua_getglobal(L, "lua_worker");   // Push function onto stack
		if (!lua_isfunction(L, -1))
		{	assert(false);
			fprintf(stderr, "Lua error: lua_worker not found\n");
			lua_pop(L, 1);
			return false;
		}

		// Execute Lua function with no args and no return values
		if (lua_pcall(L, 0, 1, 0) != LUA_OK)
		{	assert(false);
			fprintf(stderr, "Lua error: %s\n", lua_tostring(L, -1));
			lua_pop(L, 1);
			return false;
		}

		if (!lua_isinteger(L, -1))
		{	assert(false);
			fprintf(stderr, "Lua error: result is not integer\n");
			lua_pop(L, 1);
			return false;
		}
		const auto val = lua_tointeger(L, -1);
		lua_pop(L, 1);
		assert(VAL == val);

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
	bool test()
	{	TM("lua test");
		
		bool result = false;
		if (auto L = init())
		{	if (load_script(L))
			{	result = call(L);
			}

			cleanup(L);
		}

		return result;
	}

	const auto _ = register_test("Lua", test);

} // namespace lua
