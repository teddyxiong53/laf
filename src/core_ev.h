#ifndef __CORE_EV_H__
#define __CORE_EV_H__



#include "ev.h"


/* 获取用户数据 */
#define core_get_watcher_userdata(watcher) ((watcher)->data ? (watcher)->data: NULL)

/* 设置用户数据 */
#define core_set_watcher_userdata(watcher, userdata) ((watcher)->data = (userdata))



// 只处理这5种事件
typedef ev_io core_io;
typedef ev_idle core_task;//idle作为task
typedef ev_timer core_timer;
typedef ev_signal core_signal;
typedef ev_child core_child;

typedef struct ev_loop core_loop;

//五种事件对应的回调
typedef void (*_IO_CB)(core_loop *loop, core_io *io, int revents);
typedef void (*_TASK_CB)(core_loop *loop, core_task *task, int revents);
typedef void (*_TIMER_CB)(core_loop *loop, core_timer *timer, int revents);
typedef void (*_SIGNAL_CB)(core_loop *loop, core_signal *io, int revents);
typedef void (*_CHILD_CB)(core_loop *loop, core_child *io, int revents);

//io相关的
void core_io_init(core_io *io, _IO_CB cb, int fd, int events);
void core_io_start(core_loop *loop, core_io *io);
void core_io_stop(core_loop *loop, core_io *io);

//task相关的
void core_task_init(core_task *task, _TASK_CB cb);
void core_task_start(core_loop *loop, core_task *task);
void core_task_stop(core_loop *loop, core_task *task);

//timer相关的
void core_timer_init(core_timer *timer, _TIMER_CB cb);
void core_timer_start(core_loop *loop, core_timer *timer, ev_tstamp timeout);
void core_timer_stop(core_loop *loop, core_timer *timer);

//signal相关，没有stop函数
void core_signal_init(core_signal *signal, _SIGNAL_CB cb, int signum);
void core_signal_start(core_loop *loop, core_signal *signal);

//child相关的
void core_child_init(core_child *child, _CHILD_CB cb, pid_t pid, int trace);
void core_child_start(core_loop *loop, core_child *child);
void core_child_stop(core_loop *loop, core_child *child);



//其他函数
void core_break(core_loop *loop, int mode);
int core_start(core_loop *loop, int mode);

core_loop *core_loop_fork(core_loop *loop);
core_loop * core_default_loop();

#endif // __CORE_EV_H__
