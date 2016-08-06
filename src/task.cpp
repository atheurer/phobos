#include <cstdint>
#include <cstring>
#include <iostream>
#include <memory>

extern "C" {
#include <lauxlib.h>
#include <lualib.h>
}

#include <rte_launch.h>

#include "task.hpp"

#define MG_LUA_PATH "[[\
lua/?.lua;\
lua/?/init.lua;\
lua/lib/?.lua;\
lua/lib/?/init.lua;\
../lua/?.lua;\
../lua/?/init.lua;\
../lua/lib/?.lua;\
../lua/lib/?/init.lua;\
../../lua/?.lua;\
../../lua/?/init.lua;\
../../lua/lib/?.lua;\
../../lua/lib/?/init.lua;\
]]"

namespace phobos {
	lua_State* launch_lua() {
		lua_State* L = luaL_newstate();
		luaL_openlibs(L);
		luaL_dostring(L, "package.path = package.path .. ';' .. " MG_LUA_PATH);
		if (luaL_dostring(L, "require 'main'")) {
			std::cerr << "Could not run main script: " << lua_tostring(L, -1) << std::endl;
			std::abort();
		}
		return L;
	}

	int lua_core_main(void* arg) {
		std::unique_ptr<const char[]> arg_str;
		arg_str.reset(reinterpret_cast<const char*>(arg));
		lua_State* L = launch_lua();
		if (!L) {
			return -1;
		}
		lua_getglobal(L, "main");
		lua_pushstring(L, "slave");
		lua_pushstring(L, arg_str.get());
		if (lua_pcall(L, 2, 0, 0)) {
			std::cerr << "Lua error: " << lua_tostring(L, -1) << std::endl;
			return -1;
		}
		return 0;
	}

}

extern "C" {
	void launch_lua_core(int core, const char* arg) {
		size_t arg_len = strlen(arg);
		char* arg_copy = new char[arg_len + 1];
		strcpy(arg_copy, arg);
		rte_eal_remote_launch(&phobos::lua_core_main, arg_copy, core);
	}
}
