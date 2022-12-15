#include "lauxlib.h"
#include "lua.h"
#include "lualib.h"
#include "mylog.h"
#include "mylualib.h"
#include <time.h>

// #define LUALIBS_PATH \
//   "../lualib/?.lua;;../lualib/?/init.lua;;"       \
//   "../lualib/?.lc;;../lualib/?/init.lc;;"

// #define LUACLIBS_PATH \
//   "../luaclib/.libs/?.so;;../luaclib/.libs/lib?.so;;"

void init_lua_libs(lua_State* L)
{
    // 打开标准库
    luaL_openlibs(L);
    /* 注入lua搜索域 */
    lua_getglobal(L,"package");
	lua_getfield(L,-1,"preload");
	lua_pushcfunction(L,luaopen_sys);
	lua_setfield(L,-2,"sys");
    lua_pushcfunction(L,luaopen_timer);
	lua_setfield(L,-2,"timer");
}
int main(void)
{
    lua_State* L = luaL_newstate();
    init_lua_libs(L);
    int ret = 0;
    ret = luaL_dofile(L, "./test-mylib.lua");
    if (ret) {
        myloge("execute lua file fail, %d", ret);
    }
    lua_close(L);
    return 0;
}