#include "tinyev.h"
#include "mylog.h"

static int ev_default_ctx_inited = 0;
static int quit_sig = 0;
static ev_context ev_default_ctx;

static void ev_signal_int_handler(int signo)
{
    eventfd_write(quit_sig, 1);
}

static void ev_stop_callback(ev_context *ctx, void *arg)
{
    ev_stop(ctx);
}

int ev_init(ev_context *ctx, int num)
{
    int ret = ev_api_init(ctx, num);
    if (ret < 0) {
        myloge("ev_api_init fail");
        goto fail;
    }
    ctx->stop = 0;
    ctx->fired_events = 0;
    ctx->is_running = 0;
    ctx->maxevents = num;
    ctx->events_nr = num;
    ctx->events_mointored = calloc(num, sizeof(struct ev));
    mylogd("ev_init ok");
    return 0;
fail:
    return -1;
}

void ev_add_monitored(ev_context *ctx, int fd, int mask, ev_callback_t callback, void *data)
{
    //如果fd打印当前分配的maxevents，则进行扩展
    // mylogd("fd:%d, maxevents:%d", fd, ctx->maxevents);
    if (fd >= ctx->maxevents) {
        //TODO
    }
    ctx->events_mointored[fd].fd = fd;
    ctx->events_mointored[fd].mask |= mask;//这里是|=,不会清空本来的mask。
    if (mask & EV_READ) {
        ctx->events_mointored[fd].rdata = data;
        ctx->events_mointored[fd].rcallback = callback;
    }
    if (mask & EV_WRITE) {
        ctx->events_mointored[fd].wdata = data;
        ctx->events_mointored[fd].wcallback = callback;
    }
}

int ev_register_event(ev_context *ctx, int fd, int mask, ev_callback_t callback, void *data)
{
    ev_add_monitored(ctx, fd, mask, callback, data);
    int ret = 0;
    ret = ev_api_register_event(ctx, fd, mask);

    if (ret < 0) {
        myloge("ev_api_register_event fail");
        goto fail;
    }
    if (mask & EV_EVENTFD) {
        eventfd_write(fd, 1);
    }

    return 0;
fail:
    return -1;
}

int ev_get_event_type(ev_context *ctx, int idx)
{
    return ev_api_get_event_type(ctx, idx);
}

int ev_process_event(ev_context *ctx, int idx, int mask)
{
    if (mask == EV_NONE) {
        mylogd("none events happens on idx:%d\n", idx);
        return 0;
    }
    // mylogd("idx:%d", idx);
    struct ev *e = ev_api_fetch_event(ctx, idx, mask);
    if (e == NULL) {
        myloge("e is NULL");
        return -1;
    }
    // mylogd("mask:0x%08x", mask);
    int err = 0;
    int fired = 0;
    int fd = e->fd;
    eventfd_t evfd_zero = 0;
    unsigned long ul_zero = 0;
    if (mask & EV_CLOSEFD) {
        err = eventfd_read(fd, &evfd_zero);
        if (err < 0) {
            mylogd("eventfd_read ret:%d\n", err);
            return 0;//just return ok
        }
        e->rcallback(ctx, e->rdata);
        fired++;
    } else {
        if (mask & EV_EVENTFD) {
            err = eventfd_read(fd, &evfd_zero);
            close(fd);//why close it ?
        } else if (mask & EV_TIMERFD) {
            err = read(fd, &ul_zero, sizeof(ul_zero));
        }
        if (err < 0) {
            mylogd("err:%d", err);
            return 0;//return ok
        }
        if (mask & EV_READ) {

            e->rcallback(ctx, e->rdata);
            fired ++;
        }
        if (mask & EV_WRITE) {

            if (!fired || e->wcallback != e->rcallback) {//为什么要加上这个条件？
                e->wcallback(ctx, e->wdata);
                fired ++;
            }

        }
    }

    return fired;
}
int ev_poll(ev_context *ctx, time_t to)
{
    return ev_api_poll(ctx, to);
}
int ev_run(ev_context *ctx)
{
    int n = 0;
    int events = 0;
    ctx->is_running = 1;
    int i = 0;
    while (!ctx->stop) {

        n = ev_poll(ctx, -1);
        // mylogd("n:%d",n);
        if (n < 0) {
            if (errno == EINTR) {
                myloge("err EINTR, try again");
                continue;
            } else {
                myloge("critical error:%s, just break loop", strerror(errno));
                break;
            }
        }
        for (i=0; i<n; i++) {
            events = ev_get_event_type(ctx, i);
            // mylogd("events:0x%08x", events);
            ctx->fired_events += ev_process_event(ctx, i, events);

        }

    }
    return n;
}

void ev_stop(ev_context *ctx)
{
    ctx->is_running = 0;
    ctx->stop = 1;
}

int ev_fire_event(ev_context *ctx, int fd, int mask, ev_callback_t callback, void *data)
{
    int ret = 0;
    ev_add_monitored(ctx, fd, mask, callback, data);
    ret = ev_api_fire_event(ctx, fd, mask);
    if (ret < 0) {
        myloge("ev_api_fire_event fail");
        return -1;
    }
    //TODO：eventfd不应该被区分处理。
    if (mask & EV_EVENTFD) {
        ret = eventfd_write(fd, 1);
        if (ret < 0) {
            myloge("eventfd_write fail");
            return -1;
        }
    }
    return 0;
}

int ev_del_fd(ev_context *ctx, int fd)
{
    memset(&ctx->events_mointored[fd], 0, sizeof(struct ev));
    return ev_api_del_fd(ctx, fd);
}

void ev_destroy(ev_context *ctx)
{
    int i;
    for (i=0; i<ctx->maxevents; i++) {
        if (!(ctx->events_mointored[i].mask & EV_CLOSEFD)
            && (ctx->events_mointored[i].mask != EV_NONE)
        ) {
            ev_del_fd(ctx, ctx->events_mointored[i].fd);
        }
    }
    ctx->is_running = 0;
    free(ctx->events_mointored);
    ev_api_destroy(ctx);
}

int ev_is_running(ev_context *ctx)
{
    return ctx->is_running;
}
/*
    注册了signal的处理。靠一个eventfd来处理。
    这个不太合适。就用一个signalfd处理不就好了吗？
    eventfd也不应该被特殊处理。当成普通的fd处理不就好了？
*/
ev_context * ev_get_ev_context(void)
{
    if (ev_default_ctx_inited == 0) {
        quit_sig = eventfd(0, EFD_NONBLOCK);
        signal(SIGINT, ev_signal_int_handler);
        signal(SIGTERM, ev_signal_int_handler);
        int ret = 0;
        ret = ev_init(&ev_default_ctx, EVENTLOOP_MAX_EVENTS);
        if (ret < 0) {
            myloge("ev_init fail");
            return NULL;
        }
        ev_register_event(&ev_default_ctx, quit_sig, EV_CLOSEFD|EV_READ, ev_stop_callback, NULL);
        ev_default_ctx_inited = 1;
    }
    return &ev_default_ctx;
}