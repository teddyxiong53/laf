#include "lua.h"
#include "lualib.h"
#include "lauxlib.h"
#include <time.h>

char *timer()
{
    return "timer";
}

static int ltimer(lua_State *L)
{
    lua_pushstring(L, timer());
    return 1;
}

luaL_Reg timer_libs[] = {
        {"timer", ltimer},
        {NULL, NULL}
};
int luaopen_timer(lua_State *L)
{
	luaL_newlib(L, timer_libs);
    luaL_setfuncs(L, timer_libs, 0);
    return 1;
}