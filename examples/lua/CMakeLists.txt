cmake_minimum_required(VERSION 3.15)
project(lua LANGUAGES C VERSION 5.48)

# Collect all Lua source and header files from lua/
file(GLOB LUA_CORE lua/*.c lua/*.h)
# Remove lua.c and luac.c from the list of core sources
list(REMOVE_ITEM LUA_CORE "lua/lua.c" "lua/luac.c")

# Build static library liblua.a from core sources
add_library(lua_core STATIC ${LUA_CORE})
target_include_directories(lua_core
INTERFACE
	${CMAKE_CURRENT_SOURCE_DIR}
)
set_target_properties(lua_core
PROPERTIES
	FOLDER "Examples/Lua"
	OUTPUT_NAME "lua_core$<IF:$<CONFIG:Debug>,_d,>"
	OUTPUT_NAME_DEBUG "lua_core_d"
	OUTPUT_NAME_RELEASE "lua_core"
	POSITION_INDEPENDENT_CODE ON
)

# Build Lua interpreter executable
add_executable(lua "lua/lua.c")
target_link_libraries(lua PRIVATE lua_core)
if(UNIX AND NOT APPLE)
	target_link_libraries(lua PRIVATE m)
endif()

set_target_properties(lua
PROPERTIES
	FOLDER "Examples/Lua"
	OUTPUT_NAME "lua$<IF:$<CONFIG:Debug>,_d,>"
	OUTPUT_NAME_DEBUG "lua_d"
	OUTPUT_NAME_RELEASE "lua"
	POSITION_INDEPENDENT_CODE ON
)
