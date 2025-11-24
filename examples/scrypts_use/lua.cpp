#include <iostream>
#include <string>

// Include Lua headers (assumes Lua 5.x)
extern "C" {
    #include "lua.h"
    #include "lualib.h"
    #include "lauxlib.h"
}

// ---------------------------------------------------------
// 1. C++ Function to be called from Lua
// ---------------------------------------------------------
// Lua C functions must match the signature: int (*)(lua_State*)
static int cpp_callback(lua_State* L) {
    // Check if the first argument is a string and get it
    const char* message = luaL_checkstring(L, 1);
    
    std::cout << "[C++] Callback received: " << message << std::endl;
    
    // Push return value (an integer) onto the stack
    lua_pushinteger(L, 42);
    
    // Return the number of results pushed (1)
    return 1;
}

// ---------------------------------------------------------
// 2. Main Application
// ---------------------------------------------------------
int main(int argc, char* argv[]) {
    std::cout << "[C++] Starting application..." << std::endl;

    // Create a new Lua state
    lua_State* L = luaL_newstate();
    
    // Load standard Lua libraries (print, string, math, etc.)
    luaL_openlibs(L);

    // Register the C++ function as a global in Lua
    // "cpp_callback" is the name used inside Lua
    lua_register(L, "cpp_callback", cpp_callback);

    // Define Lua script inline
    const char* lua_script = R"(
        function lua_worker()
            print('[Lua] Inside lua_worker function')
            
            print('[Lua] Calling C++ callback...')
            local result = cpp_callback('Hello from Lua!')
            
            print('[Lua] Callback returned: ' .. tostring(result))
            print('[Lua] Finishing lua_worker')
        end
    )";

    // Load and compile the string (pushes the chunk onto stack), then run it
    if (luaL_dostring(L, lua_script) != LUA_OK) {
        std::cerr << "Error loading script: " << lua_tostring(L, -1) << std::endl;
        lua_close(L);
        return 1;
    }

    // --- THE FULL CYCLE HAPPENS HERE ---
    std::cout << "[C++] Calling Lua function 'lua_worker'..." << std::endl;

    // Push the global function name onto the stack
    lua_getglobal(L, "lua_worker");

    // Check if it's actually a function
    if (lua_isfunction(L, -1)) {
        // Call the function: 0 arguments, 0 results, 0 error handler
        if (lua_pcall(L, 0, 0, 0) != LUA_OK) {
            std::cerr << "Error calling function: " << lua_tostring(L, -1) << std::endl;
        } else {
            std::cout << "[C++] Lua function returned successfully." << std::endl;
        }
    } else {
        std::cerr << "Function 'lua_worker' not found" << std::endl;
    }

    // Cleanup
    lua_close(L);

    std::cout << "[C++] Exiting." << std::endl;
    return 0;
}