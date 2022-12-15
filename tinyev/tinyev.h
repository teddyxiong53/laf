#ifndef __TINYEV_H__
#define __TINYEV_H__

#include <time.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>
#include <unistd.h>
#include <signal.h>
#include <stdio.h>
#include <netdb.h>
#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>

#include <sys/socket.h>

#include <sys/eventfd.h>
#include <sys/timerfd.h>
#include <sys/types.h>
#include <sys/epoll.h>

#define TINYEV_USE_EPOLL 1
#define TINYEV_USE_POLL 0
#define TINYEV_USE_SELECT 0

#if TINYEV_USE_EPOLL
struct epoll_api {
    int fd;
    struct epoll_event *events;
};

#endif

#define EVENTLOOP_MAX_EVENTS    1024

#define EVENTLOOP_TIMEOUT       -1

#define EV_OK   0
#define EV_ERR  -1
#define EV_OOM  -2

enum ev_type {
    EV_NONE       = 0x00,
    EV_READ       = 0x01,
    EV_WRITE      = 0x02,
    EV_DISCONNECT = 0x04,
    EV_EVENTFD    = 0x08,
    EV_TIMERFD    = 0x10,
    EV_CLOSEFD    = 0x20
};


typedef struct ev_ctx {
    int events_nr;
    int maxfd;//
    int stop;
    int is_running;
    int maxevents;
    uint64_t fired_events;
    struct ev *events_mointored;
    void *api; //指向特定的backend的指针。
} ev_context;
typedef void (*ev_callback_t)(ev_context *ctx, void *arg);

struct ev {
    int fd;
    int mask;
    void *rdata;//read cb的参数
    void *wdata;//write cb的参数
    // void (*rcallback)(ev_context *ctx, void *arg);// read cb
    // void (*wcallback)(ev_context *ctx, void *arg);//write cb
    ev_callback_t rcallback;
    ev_callback_t wcallback;
};


int ev_init(ev_context *ctx, int num);
int ev_is_running(ev_context *ctx);
ev_context * ev_get_ev_context(void);

int ev_register_event(ev_context *ctx, int fd, int mask, ev_callback_t callback, void *data);
int ev_fire_event(ev_context *ctx, int fd, int mask, ev_callback_t callback, void *data);
int ev_poll(ev_context *ctx, time_t to);
int ev_run(ev_context *ctx);
void ev_stop(ev_context *ctx);
void ev_destroy(ev_context *ctx);
int ev_del_fd(ev_context *ctx, int fd);

int ev_api_init(ev_context *ctx, int num);
int ev_api_register_event(ev_context *ctx, int fd, int mask);
int ev_api_get_event_type(ev_context *ctx, int idx);
struct ev * ev_api_fetch_event(ev_context *ctx, int idx, int mask);
int ev_api_fire_event(ev_context *ctx, int fd, int mask);
int ev_api_del_fd(ev_context *ctx, int fd);
int ev_api_poll(ev_context *ctx, time_t timeout);
void ev_api_destroy(ev_context *ctx);

#endif // __TINYEV_H__
