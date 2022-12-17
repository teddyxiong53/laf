#include "lauxlib.h"
#include "lua.h"
#include "lualib.h"
#include <time.h>
#include "mylog.h"
#include "core.h"
/* 微秒级时间戳函数 */
double now()
{
    struct timespec now = {};
    clock_gettime(CLOCK_REALTIME, &now);
    return now.tv_sec + now.tv_nsec * 1e-9;
}

/* 检查是否为有效ipv4地址 */
int ipv4(const char *IP)
{

}

/* 检查是否为有效ipv6地址 */
int ipv6(const char *IP)
{

}

/* 返回当前操作系统类型 */
const char* os()
{
    return "Linux";
}


static int los(lua_State* L)
{
    lua_pushstring(L, os());
    return 1;
}


static int lnow(lua_State* L)
{
    lua_pushnumber(L, now());
    return 1;
}
luaL_Reg sys_libs[] = {
    { "os", los },
    { "now", lnow },
    { NULL, NULL }
};

int luaopen_sys(lua_State* L)
{
    mylogd("");
    luaL_newlib(L, sys_libs);
    // luaL_setfuncs(L, sys_libs, 0);luaL_newlib是一个宏，包括了luaL_setfuncs
    return 1;
}