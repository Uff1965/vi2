#include "header.h"

#include <vi_timing/vi_timing.h>

// Include Lua headers (assumes Lua 5.x)
extern "C" {
	#include <lua/lua.h>
	#include <lua/lualib.h>
	#include <lua/lauxlib.h>
}

VI_TM(FILE_PATH);

namespace lua
{
	// 1. C++ Function to be called from Lua
	static int cpp_callback(lua_State *L)
	{	VI_TM_FUNC;
		luaL_checkstring(L, 1); // Check if the first argument is a string and get it
		lua_pushinteger(L, 42); // Push return value (an integer) onto the stack
		return 1; // Return the number of results pushed (1)
	}

	// 2. Main Application
	void test_lua(void)
	{	VI_TM_FUNC;

		lua_State *L = nullptr;

		{	VI_TM("Lua(1) Initialize");
			L = luaL_newstate(); // Create a new Lua state
			luaL_openlibs(L); // Load standard Lua libraries (print, string, math, etc.)

			// Register the C++ function as a global in Lua
			// "cpp_callback" is the name used inside Lua
			lua_register(L, "cpp_callback", cpp_callback);
		}

		{	VI_TM("Lua(2) Load and compile");
			// Define Lua script inline
			const char *lua_script =
				"function lua_worker()"
				"local result = cpp_callback('Hello from Lua!')"
				"end";

			// Load and compile the string (pushes the chunk onto stack), then run it
			if (luaL_dostring(L, lua_script) != LUA_OK)
			{
				lua_close(L);
				return;
			}
		}

		{	VI_TM("Lua(3) Call");
			// --- THE FULL CYCLE HAPPENS HERE ---
			lua_getglobal(L, "lua_worker"); // Push the global function name onto the stack

			// Check if it's actually a function
			if (lua_isfunction(L, -1))
			{	// Call the function: 0 arguments, 0 results, 0 error handler
				if (lua_pcall(L, 0, 0, 0) != LUA_OK)
				{	assert(false);
				}
			}
			else
			{	assert(false);
			}
		}

		{	VI_TM("Lua(4) Cleanup");
			// Cleanup is done below
			lua_close(L); // Cleanup
		}

		return;
	}

	const auto _ = register_test(test_lua);

} // namespace lua
