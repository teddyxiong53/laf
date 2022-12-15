#include "tinyev.h"
#include "mylog.h"

#if TINYEV_USE_EPOLL

static int epoll_add(int efd, int fd, int evs, void *data)
{

    struct epoll_event ev;
    ev.data.fd = fd;
    if (data) {//data是一个union，这样会让fd变成一个不确定的值，要注意，不过当前data都是为NULL的。
        ev.data.ptr = data;
    }
    ev.events = evs | EPOLLHUP | EPOLLERR;
    return epoll_ctl(efd, EPOLL_CTL_ADD, fd, &ev);
}

static int epoll_mod(int efd, int fd, int evs, void *data)
{
    struct epoll_event ev;
    ev.data.fd = fd;
    if (data) {
        ev.data.ptr = data;
    }
    ev.events = evs | EPOLLHUP | EPOLLERR;
    int ret= epoll_ctl(efd, EPOLL_CTL_MOD, fd, &ev);
    // mylogd("ret:%d", ret);
    return ret;
}

static int epoll_del(int efd, int fd)
{
    return epoll_ctl(efd, EPOLL_CTL_DEL, fd, NULL);
}


int ev_api_init(ev_context *ctx, int num)
{
    struct epoll_api *api = malloc(sizeof(*api));
    if (!api) {
        myloge("malloc fail");
        return -1;
    }
    api->fd = epoll_create1(0);
    api->events = calloc(num, sizeof(struct epoll_event));
    ctx->api = api;
    ctx->maxfd = num;
    return 0;
}

int ev_api_register_event(ev_context *ctx, int fd, int mask)
{
    struct epoll_api *api = ctx->api;
    int op = 0;
    if (mask & EV_READ) {
        op |= EPOLLIN;
    }
    if (mask & EV_WRITE) {
        op |= EPOLLOUT;
    }

    int ret = epoll_add(api->fd, fd, op, NULL);
    // mylogd("ret:%d", ret);

    return ret;
}

int ev_api_get_event_type(ev_context *ctx, int idx)
{
    struct epoll_api *api = ctx->api;
    int events = api->events[idx].events;
    int fd = api->events[idx].data.fd;
    int ev_mask = ctx->events_mointored[fd].mask;
    int mask = 0;
    if (ev_mask & (EV_CLOSEFD | EV_TIMERFD)) {
        mask = ev_mask;
    } else {
        mask = EV_NONE;
    }
    if (events & (EPOLLERR | EPOLLHUP | EPOLLRDHUP)) {
        mask |= EV_DISCONNECT;
    }
    if (events& EPOLLIN) {
        mask |= EV_READ;
    }
    if (events & EPOLLOUT) {
        mask |= EV_WRITE;
    }
    return mask;
}

struct ev * ev_api_fetch_event(ev_context *ctx, int idx, int mask)
{
    int fd = 0;
    struct epoll_api *api = ctx->api;
    fd = api->events[idx].data.fd;
    // mylogd("fd:%d", fd);
    return ctx->events_mointored + fd;
}


int ev_api_fire_event(ev_context *ctx, int fd, int mask)
{
    struct epoll_api *api = ctx->api;
    int op = 0;
    int ret = 0;
    // mylogd("fd:%d, mask:0x%08x", fd, mask);
    if (mask & EV_READ) {
        op |= EPOLLIN;
    }
    if (mask & EV_WRITE) {
        op |= EPOLLOUT;
    }
    if (mask & EV_EVENTFD) {//这个是每次add，然后关闭掉。

        ret = epoll_add(api->fd, fd, op, NULL);
    } else {
        ret = epoll_mod(api->fd, fd, op, NULL);
    }
    // mylogd("ret:%d, reason:%s\n", ret, strerror(errno));
    return ret;
}


int ev_api_del_fd(ev_context *ctx, int fd)
{
    struct epoll_api *api = ctx->api;
    return epoll_del(api->fd, fd);
}

int ev_api_poll(ev_context *ctx, time_t timeout)
{
    struct epoll_api *api = ctx->api;
    // mylogd("timeout:%d", timeout);
    int ret = epoll_wait(api->fd, api->events, ctx->events_nr, timeout);
    // mylogd("ret:%d", ret);
    return ret;
}

void ev_api_destroy(ev_context *ctx)
{
    struct epoll_api *api = ctx->api;
    close(api->fd);
    free(api->events);
    free(api);
}

#endif