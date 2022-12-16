#include "core_ev.h"
#include "mylog.h"

core_loop *core_default_loop()
{
    return ev_default_loop(0);
}

void core_io_init(core_io *io, _IO_CB cb, int fd, int events)
{
    ev_io_init(io, cb, fd, events);
}
void core_io_start(core_loop *loop, core_io *io)
{
    if (loop) {
        ev_io_start(loop, io);
    } else {
        //否则使用默认的loop
        ev_io_start(core_default_loop(), io);
    }

}
void core_io_stop(core_loop *loop, core_io *io)
{
    if (io->events || io->fd) {
        ev_io_stop(loop? loop: core_default_loop(), io);
        io->fd = -1;
        io->events = -1;
    }
}

void core_task_init(core_task *task, _TASK_CB cb)
{
    ev_idle_init(task, cb);
}


void core_task_start(core_loop *loop, core_task *task)
{
    ev_idle_start(loop? loop: core_default_loop(), task);
}

void core_task_stop(core_loop *loop, core_task *task)
{
    ev_idle_stop(loop? loop: core_default_loop(), task);
}

void core_timer_init(core_timer *timer, _TIMER_CB cb)
{
    timer->repeat = 0;
    timer->at = 0;
    ev_init(timer, cb);
}
void core_timer_start(core_loop *loop, core_timer *timer, ev_tstamp timeout)
{
    timer->repeat = timeout;
    ev_timer_again(loop?loop:core_default_loop(), timer);
}

void core_timer_stop(core_loop *loop, core_timer *timer)
{
    timer->repeat = 0;
    timer->at = 0;
    ev_timer_again(loop?loop:core_default_loop(), timer);
}

void core_signal_init(core_signal *signal, _SIGNAL_CB cb, int signum)
{
    ev_signal_init(signal, cb, signum);
}
void core_signal_start(core_loop *loop, core_signal *signal)
{
    ev_signal_start(loop?loop:core_default_loop(), signal);
}


void core_child_init(core_child *child, _CHILD_CB cb, pid_t pid, int trace)
{
    ev_child_init(child, cb, pid, trace);
}
void core_child_start(core_loop *loop, core_child *child)
{
    ev_child_start(loop?loop:core_default_loop(), child);
}
void core_child_stop(core_loop *loop, core_child *child)
{
    ev_child_stop(loop?loop:core_default_loop(), child);
}

void core_break(core_loop *loop, int mode)
{
    ev_break(loop?loop:core_default_loop(), mode);
}
int core_start(core_loop *loop, int mode)
{
    ev_run(loop?loop:core_default_loop(), mode);
}

core_loop *core_loop_fork(core_loop *loop)
{
    ev_loop_fork(loop);
    return loop;
}

