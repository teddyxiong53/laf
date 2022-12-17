#include "lua.h"
#include "lualib.h"
#include "lauxlib.h"
#include <time.h>
#include "core.h"
#include "mylog.h"

static void TIMEOUT_CB(core_loop *loop, core_timer *timer, int revents)
{
    if (revents & EV_TIMER) {
        lua_State *co = (lua_State *)core_get_watcher_userdata(timer);
        mylogd("co in callback:%p", co);
        mylogd("timer:%p", timer);
        int ret = lua_resume(co, NULL, lua_gettop(co)>0? lua_gettop(co)-1: 0);
        if ((ret != LUA_OK) && (ret != LUA_YIELD)) {
            myloge("lua resume fail:%s", lua_tostring(co, -1));
            core_timer_stop(loop, timer);
        }
    }
}

static int timer_stop(lua_State *L)
{
    core_timer *timer = (core_timer *)luaL_testudata(L, 1, "__TIMER__");
    if (!timer) {
        myloge("timer is null");
        return 0;
    }
    core_timer_stop(core_default_loop(), timer);
}

static int timer_start(lua_State *L)
{
    core_timer *timer = (core_timer *)luaL_testudata(L, 1, "__TIMER__");
    if (!timer) {
        myloge("timer is null");
        return 0;
    }
    lua_Number timeout = luaL_checknumber(L, 2);
    if (timeout <= 0) {
        myloge("invalid timeout val");
        return 0;
    }
    lua_State *co = lua_tothread(L, 3);//拿到这个co，后面要在callback里resume回来。
    if (!co) {
        myloge("co is null");
        return 0;
    }
    mylogd("co in start:%p", co);
    core_set_watcher_userdata(timer, (void *)co);
    core_timer_start(core_default_loop(), timer, timeout);
    return 0;//没有数据要入栈。所以返回0
}

static int timer_new(lua_State *L)
{
    core_timer *timer = (core_timer *)lua_newuserdata(L, sizeof(core_timer));
    if (!timer) {
        myloge("malloc fail");
        return 0;
    }
    core_timer_init(timer, TIMEOUT_CB);
    luaL_setmetatable(L, "__TIMER__");
    return 1;
}

static luaL_Reg timer_libs[] = {
        {"new", timer_new},
        {"start", timer_start},
        {"stop", timer_stop},
        {NULL, NULL}
};
int luaopen_timer(lua_State *L)
{
    luaL_newmetatable(L, "__TIMER__");
    lua_pushstring(L, "__index");
    lua_pushvalue(L, -2);
    lua_rawset(L, -3);

    lua_pushliteral(L, "__mode");
    lua_pushliteral(L, "kv");
    lua_rawset(L, -3);

	luaL_newlib(L, timer_libs);
    return 1;
}