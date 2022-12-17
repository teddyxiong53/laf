#include "core.h"
#include "mylog.h"

//在Makefile.am里定义这个宏
char script_entry[1024] = LUA_ENTRY;
#pragma message(PRINT_MACRO(LUA_ENTRY))

static int traceback( lua_State *L)
{
    const char *msg = lua_tostring(L,-1);
    mylogd("msg:%s", msg);
    if (msg)
    {
        /*打印发生错误时的堆栈信息,msg是错误信息,附加到打印的堆栈最前端
        1是指从第一层开始回溯
        这个函数之后,lua_tostring(L,-1)即可获取完整的堆栈和错误信息.
        */
        luaL_traceback(L,L,msg,1);
    }
    else
    {
        lua_pushliteral(L,"no message");
    }
}


int core_worker_run(const char entry[])
{
    mylogd("");
    core_loop *loop = core_loop_fork(core_default_loop());
    int ret = 0;
    lua_State *L = luaL_newstate();
    if (!L) {
        myloge("create L fail");
        exit(-1);
    }
    mylogd("");
    //初始化标准库和自定义的库
    init_lua_libs(L, 2);
    mylogd("");
    mylogd("lua top:%d", lua_gettop(L));
    lua_pushcfunction(L,traceback);
    mylogd("lua top:%d", lua_gettop(L));
    int r = luaL_loadfile(L,entry);
    mylogd("lua top:%d", lua_gettop(L));
    //第四个参数表示将错误信息msg传入栈1所在的函数(错误处理函数)
    r = lua_pcall(L,0,0,1);
    if (r != LUA_OK)
        printf("call err\n%s\n", lua_tostring(L,-1));
    else
        printf("call succ\n");
    //load 入口文件
    // luaL_loadfile(L, entry);
    // luaL_dofile(L, entry);
    mylogd("");
    //阻塞死循环这里
    return core_start(loop, 0);
}

/* 多进程 - 运行工作进程 */
static inline int cfadmin_worker_run(const char* entry)
{
    return core_worker_run(entry);
}

/* 单进程模式 */
static inline int cfadmin_standalone_run(const char* entry)
{
    return cfadmin_worker_run(entry);
}

int main(int argc, char const* argv[])
{
    return cfadmin_standalone_run(script_entry);
}
