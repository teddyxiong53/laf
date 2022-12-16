#include "core.h"
#include "mylog.h"

//在Makefile.am里定义这个宏
char script_entry[1024] = LUA_ENTRY;
#pragma message(PRINT_MACRO(LUA_ENTRY))

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
    //load 入口文件
    luaL_loadfile(L, entry);
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
