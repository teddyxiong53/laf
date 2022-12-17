#include "core.h"
#include "mylog.h"

void init_lua_mode(lua_State *L, int mode)
{
    if (mode == 2) {
        lua_pushliteral(L, "worker");
        //创建一张新的空表压栈
        lua_createtable(L, 0, 0);
        //往表里加入id以及它的值。
        lua_pushliteral(L, "id");
        lua_pushinteger(L, getpid() - getppid());
        lua_rawset(L, -3);//一次出栈出栈，需要写入到lua。

        lua_pushliteral(L, "pid");
        lua_pushinteger(L, getpid());
        lua_rawset(L, -3);

        lua_pushliteral(L, "ppid");
        lua_pushinteger(L, getppid());
        lua_rawset(L, -3);

        lua_rawset(L, -3);//相当于把table整个写入给lua
    } else {

    }
}
#pragma message(PRINT_MACRO(LUALIBS_PATH))
#pragma message(PRINT_MACRO(LUACLIBS_PATH))

void init_lua_libs(lua_State *L, int mode)
{
    //打开标准库
    luaL_openlibs(L);
    //将全局环境压栈。
    lua_pushglobaltable(L);
    //给lua增加null和NULL的值
    lua_pushliteral(L, "null");
    lua_pushlightuserdata(L, NULL);
    lua_rawset(L, -3);
    lua_pushliteral(L, "NULL");
    lua_pushlightuserdata(L, NULL);
    lua_rawset(L, -3);

    init_lua_mode(L, mode);

    lua_settop(L, 0);//把栈上的所有内容移除。因为设置的内容都已经写给lua了。所以移除没有什么问题。

    //接下来进行注入lua搜索域操作
    lua_getglobal(L, "package");//先把package这个表从lua拿出来。
    //然后设置它的path属性。
    lua_pushliteral(L, LUALIBS_PATH);//这个从Makefile.am里生成
    lua_setfield(L, 1, "path");//这个会自动进行一次出栈操作。保证生效。
    //然后设置他的cpath属性。
    mylogd("cpath:%s", LUACLIBS_PATH);
    lua_pushliteral(L, LUACLIBS_PATH);
    lua_setfield(L, 1, "cpath");
    //清空栈
    lua_settop(L, 0);

}