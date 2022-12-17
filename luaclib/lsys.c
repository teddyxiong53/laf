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
static int ltime(lua_State* L)
{
    lua_pushnumber(L, (uint64_t)(now()*1e3));
    return 1;
}
static int ldate(lua_State* L)
{
    size_t str_len = 0;
    //lua调用的时候，传递的第一个参数就是格式字符串。
    char *fmt = luaL_checklstring(L, 1, &str_len);
    if (!fmt || str_len == 0) {
        myloge("date fmt is invalid");
        return luaL_error(L, "fmt is invalid");
    }
    time_t timestamp = lua_tointeger(L, 2);//第二个参数就是时间值
    if (timestamp < 0) {
        timestamp = time(NULL);
    }
    size_t len = 128 + str_len;
    char fmttime[len] = {0};
    int ret = strftime(fmttime, len, fmt, localtime(&timestamp));
    if (ret < 0) {
        myloge("fmt time fail");
        return 0;
    }
    lua_pushlstring(L, fmttime, ret);
    return 1;

}
static int lusage(lua_State* L)
{
    return 0;
}

static int lnew_tab(lua_State* L)
{
    lua_createtable(L, luaL_checkinteger(L, 1), luaL_checkinteger(L, 2));
    return 1;
}

luaL_Reg sys_libs[] = {
    { "os", los },
    { "now", lnow },
    {"time", ltime},
    {"date", ldate},
    {"usage", lusage},
    {"new_tab", lnew_tab},
    { NULL, NULL }
};

int luaopen_sys(lua_State* L)
{
    mylogd("");
    luaL_newlib(L, sys_libs);
    // luaL_setfuncs(L, sys_libs, 0);luaL_newlib是一个宏，包括了luaL_setfuncs
    return 1;
}