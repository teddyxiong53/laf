#ifndef __CORE_SYS_H__
#define __CORE_SYS_H__

#include <stdio.h>
#include <stddef.h>
#include <stdint.h>
#include <stdarg.h>
#include <string.h>
#include <unistd.h>

#include <assert.h>
#include <math.h>
#include <limits.h>
#include <time.h>
#include <errno.h>

#include <netdb.h>
#include <netinet/tcp.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/un.h>
#include <sys/time.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <fcntl.h>

#include <lua.h>
#include <lualib.h>
#include <lauxlib.h>


// 设置子进程自动关闭进程fd.
#define non_exec(socket) {fcntl(socket, F_SETFD, fcntl(socket, F_GETFL, 0) | FD_CLOEXEC);}

// 设置非阻塞模式
#define non_blocking(socket) {non_exec(socket); fcntl(socket, F_SETFL, fcntl(socket, F_GETFL, 0) | O_NONBLOCK);}

// 设置nodelay模式
#define non_delay(socket) ({int Enable = 1; setsockopt(socket, IPPROTO_TCP, TCP_NODELAY, &Enable, sizeof(Enable));})

/* 微秒级时间戳函数 */
double now(void);

/* 检查是否为有效ipv4地址 */
int ipv4(const char *IP);

/* 检查是否为有效ipv6地址 */
int ipv6(const char *IP);

/* 返回当前操作系统类型 */
const char* os(void);


#endif // __CORE_SYS_H__
