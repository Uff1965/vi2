#include "header.h"

extern "C" {
	#include <lua/lua.h>
	#include <lua/lualib.h>
	#include <lua/lauxlib.h>
}

#include <cstdio>

namespace lua
{
	// C++ function exposed to Lua
	// This simulates a native callback that Lua can invoke.
	int callback(lua_State *L)
	{	TM("0: Lua callback");
		const char *message = luaL_checkstring(L, 1);
		const auto value = luaL_checkinteger(L, 2);

		lua_Integer res = -1;
		if (const auto len = message ? strlen(message) : 0; len > 0)
		{	res = message[(value - 777) % len];
		}

		lua_pushinteger(L, res);
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
		lua_register(L, "callback", callback); // Register native function
		return L;
	}

	// Step 2: Load and compile Lua script
	bool load_script(lua_State *L)
	{   TM("2: Lua Load and compile");
		static constexpr char script[] = R"(
				function Worker(msg, val)
					return callback(msg, val + 777)
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

	int call_worker(lua_State *L, const char* msg, int val)
	{	lua_getglobal(L, "Worker"); // Push function onto stack
		if (!lua_isfunction(L, -1))
		{	assert(false);
			fprintf(stderr, "Lua error: Worker not found\n");
			lua_pop(L, 1);
			return -1;
		}
		lua_pushstring(L, msg);
		lua_pushinteger(L, val);

		if (lua_pcall(L, 2, 1, 0) != LUA_OK)
		{	assert(false);
			fprintf(stderr, "Lua error: %s\n", lua_tostring(L, -1));
			lua_pop(L, 1);
			return -1;
		}

		if (!lua_isinteger(L, -1))
		{	assert(false);
			fprintf(stderr, "Lua error: result is not integer\n");
			lua_pop(L, 1);
			return -1;
		}
		const auto result = lua_tointeger(L, -1);
		lua_pop(L, 1);

		return static_cast<int>(result);
	}

	// Step 3: Call Lua function
	bool call(lua_State *L)
	{
		{	TM("3.1: Lua First Call");
			if (MSG[0] != call_worker(L, MSG, 0))
			{	assert(false);
				return false;
			}
		}

		for(int n = 0; n < 100; ++n)
		{	TM("3.2: Lua Other Call");
			if (MSG[n % (strlen(MSG))] != call_worker(L, MSG, n))
			{	assert(false);
				return false;
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
	{	TM("*LUA test");
		
		bool result = false;
		if (auto L = init())
		{	if (load_script(L))
			{	result = call(L);
			}

			cleanup(L);
		}

		return result;
	}

	const auto _ = register_test("LUA", test);

} // namespace lua
