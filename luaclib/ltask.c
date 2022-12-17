#include "core.h"
#include "mylog.h"

static void TASK_CB(core_loop* loop, core_task* task, int revents)
{
    lua_State* co = (lua_State*)core_get_watcher_userdata(task);
    if (co && ((lua_status(co) == LUA_YIELD) || (lua_status(co) == LUA_OK))) {
        int nargs = 0;
        if (lua_status(co) == LUA_YIELD) {
            nargs = lua_gettop(co);
        } else {
            nargs = lua_gettop(co) - 1;
        }
        int ret = 0;
        ret = CO_RESUME(co, NULL, nargs);
        if ((ret != LUA_YIELD) && (ret != LUA_OK)) {
            myloge("error :%s", lua_tostring(co, -1));
        }
        core_task_stop(loop, task);
    }
}

static int task_new(lua_State* L)
{
    mylogd("");
    core_task* task = lua_newuserdata(L, sizeof(core_task));
    if (!task) {
        myloge("malloc fail");
        return 0;
    }
    core_task_init(task, TASK_CB);
    luaL_setmetatable(L, "__Task__");
    return 1;
}

static int task_start(lua_State *L)
{
    mylogd("");
    core_task *task = (core_task *)luaL_testudata(L, 1, "__Task__");
    if (!task) {
        myloge("is not task type");
        return luaL_error(L, "is not task type");
    }
    lua_State *co = lua_tothread(L, 2);
    if (!co) {
        myloge("is not lua_State value");
        return luaL_error(L, "is not lus_State value");
    }
    lua_xmove(L, co, lua_gettop(L)-2);//交互2个State里n个值，从L里弹出，压入到co里。
    core_set_watcher_userdata(task, co);
    core_task_start(core_default_loop(), task);
    return 1;
}

static int task_stop(lua_State *L)
{
    mylogd("");
    core_task *task = (core_task *)luaL_testudata(L, 1, "__Task__");
    if (!task) {
        myloge("is not task type");
        return luaL_error(L, "is not task type");
    }
    core_task_stop(core_default_loop(), task);
}

int luaopen_task(lua_State* L)
{
    mylogd("");
    luaL_newmetatable(L, "__Task__");

    lua_pushstring(L, "__index");
    lua_pushvalue(L, -2);
    lua_rawset(L, -3);

    lua_pushliteral(L, "__mode");
    lua_pushliteral(L, "kv");
    lua_rawset(L, -3);

    luaL_Reg task_libs[] = {
        { "new", task_new },
        { "start", task_start },
        { "stop", task_stop },
        { NULL, NULL }
    };
    luaL_newlib(L, task_libs);
    return 1;
}