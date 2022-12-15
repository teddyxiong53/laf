/*
这个文件留着做测试用。
*/
#ifndef EV_H
#define EV_H

#ifdef __linux__
#include <linux/version.h>
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2, 5, 44)
#define EPOLL 1
#define EVENTLOOP_BACKEND "epoll"
#elif LINUX_VERSION_CODE >= KERNEL_VERSION(2, 1, 23)
#define POLL 1
#define EVENTLOOP_BACKEND "poll"
#else
#define SELECT 1
#define EVENTLOOP_BACKEND "select"
#endif

#elif defined(__APPLE__) || defined(__FreeBSD__) \
    || defined(__OpenBSD__) || defined (__NetBSD__)
#define KQUEUE 1
#define EVENTLOOP_BACKEND "kqueue"
#else
#define SELECT 1
#define EVENTLOOP_BACKEND "select"
#endif // __linux__

#include <time.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>
#include <unistd.h>
#include <signal.h>
#ifdef __linux__
#include <sys/eventfd.h>
#include <sys/timerfd.h>
#else
#include <sys/socket.h>
#endif

/*
 * Maximum number of events to monitor at a time, useful for epoll and poll
 * calls, the value represents the length of the events array. Tweakable value.
 */
#define EVENTLOOP_MAX_EVENTS    1024

/*
 * The timeout to wait before returning on the blocking call that every IO mux
 * implementation accepts, -1 means block forever until new events arrive.
 * Tweakable value.
 */
#define EVENTLOOP_TIMEOUT       -1

/*
 * Return codes */
#define EV_OK   0
#define EV_ERR  -1
#define EV_OOM  -2

/*
 * Event types, meant to be OR-ed on a bitmask to define the type of an event
 * which can have multiple traits
 */
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
    int maxfd; // the maximum FD monitored by the event context,
               // events_monitored must be at least maxfd long
    int stop;
    int is_running;
    int maxevents;
    unsigned long long fired_events;
    struct ev *events_monitored;
    void *api; // opaque pointer to platform defined backends
} ev_context;

/*
 * Event struture used as the main carrier of clients informations, it will be
 * tracked by an array in every context created
 */
struct ev {
    int fd;
    int mask;
    void *rdata; // opaque pointer for read callback args
    void *wdata; // opaque pointer for write callback args
    void (*rcallback)(ev_context *, void *); // read callback
    void (*wcallback)(ev_context *, void *); // write callback
};

/*
 * Initialize the ev_context, accepting the number of events to monitor; that
 * value is indicative as if a FD exceeds the cap set the events array will be
 * resized.
 * The first thing done is the initialization of the api pointer according to
 * the Mux IO backend found on the host machine
 */
int ev_init(ev_context *, int);

/*
 * Just check if the ev_context is running, return 0 if it's not running, 1
 * otherwise
 */
int ev_is_running(const ev_context *);

/*
 * By design ev library can instantiate a default `ev_context`, calling
 * `ev_get_ev_context` the first time will create the loop as a singleton,
 * subsequent calls will retrieve the same first context allocated
 */
ev_context *ev_get_ev_context(void);

/*
 * Call custom destroy function based on the api type set up calling `ev_init`
 * and de-allocate all events monitored memory
 */
void ev_destroy(ev_context *);

/*
 * Poll an event context for events, accepts a timeout or block forever,
 * returning only when a list of FDs are ready to either READ, WRITE or TIMER
 * to be executed.
 */
int ev_poll(ev_context *, time_t);

/*
 * Blocks forever in a loop polling for events with ev_poll calls. At every
 * cycle executes callbacks registered with each event
 */
int ev_run(ev_context *);

/*
 * Trigger a stop on a running event, it's meant to be run as an event in a
 * running ev_ctx
 */
void ev_stop(ev_context *);

/*
 * Add a single FD to the underlying backend of the event loop. Equal to
 * ev_fire_event just without an event to be carried. Useful to add simple
 * descritors like a listening socket o message queue FD.
 */
int ev_watch_fd(ev_context *, int, int);

/*
 * Remove a FD from the loop, even tho a close syscall is sufficient to remove
 * the FD from the underlying backend such as EPOLL/SELECT, this call ensure
 * that any associated events is cleaned out an set to EV_NONE
 */
int ev_del_fd(ev_context *, int);

/*
 * Register a new event, semantically it's equal to ev_register_event but
 * it's meant to be used when an FD is not already watched by the event loop.
 * It could be easily integrated in ev_fire_event call but I prefer maintain
 * the samantic separation of responsibilities.
 */
int ev_register_event(ev_context *, int, int,
                      void (*callback)(ev_context *, void *), void *);

int ev_register_cron(ev_context *,
                     void (*callback)(ev_context *, void *),
                     void *,
                     long long, long long);

/*
 * Register a new event for the next loop cycle to a FD. Equal to ev_watch_fd
 * but allow to carry an event object for the next cycle.
 */
int ev_fire_event(ev_context *, int, int,
                  void (*callback)(ev_context *, void *), void *);

#ifdef EV_SOURCE
#ifndef EV_SOURCE_ONCE
#define EV_SOURCE_ONCE

#if defined(EPOLL)

/*
 * =========================
 *  Epoll backend functions
 * =========================
 *
 * The epoll_api structure contains the epoll fd and the events array needed to
 * wait on events with epoll_wait(2) blocking call. It's the best multiplexing
 * IO api available on Linux systems and thus the optimal choice.
 */

#include <sys/epoll.h>

struct epoll_api {
    int fd;
    struct epoll_event *events;
};

/*
 * Epoll management function, register a file descriptor to an EPOLL
 * descriptor, to be monitored for read/write events
 */
static int epoll_add(int efd, int fd, int evs, void *data) {

    struct epoll_event ev;
    ev.data.fd = fd;

    // Being ev.data a union, in case of data != NULL, fd will be set to random
    if (data)
        ev.data.ptr = data;

    ev.events = evs | EPOLLHUP | EPOLLERR;

    return epoll_ctl(efd, EPOLL_CTL_ADD, fd, &ev);
}

/*
 * Modify an epoll-monitored descriptor, can be set to EPOLLIN for read and
 * EPOLLOUT for write
 */
static int epoll_mod(int efd, int fd, int evs, void *data) {

    struct epoll_event ev;
    ev.data.fd = fd;

    // Being ev.data a union, in case of data != NULL, fd will be set to random
    if (data)
        ev.data.ptr = data;

    ev.events = evs | EPOLLHUP | EPOLLERR;

    return epoll_ctl(efd, EPOLL_CTL_MOD, fd, &ev);
}

/*
 * Remove a descriptor from an epoll descriptor, making it no-longer monitored
 * for events
 */
static int epoll_del(int efd, int fd) {
    return epoll_ctl(efd, EPOLL_CTL_DEL, fd, NULL);
}

static int ev_api_init(ev_context *ctx, int events_nr) {
    struct epoll_api *e_api = malloc(sizeof(*e_api));
    if (!e_api)
        return EV_OOM;
    e_api->fd = epoll_create1(0);
    e_api->events = calloc(events_nr, sizeof(struct epoll_event));
    ctx->api = e_api;
    ctx->maxfd = events_nr;
    return EV_OK;
}

static void ev_api_destroy(ev_context *ctx) {
    close(((struct epoll_api *) ctx->api)->fd);
    free(((struct epoll_api *) ctx->api)->events);
    free(ctx->api);
}

static int ev_api_get_event_type(ev_context *ctx, int idx) {
    struct epoll_api *e_api = ctx->api;
    int events = e_api->events[idx].events;
    int ev_mask = ctx->events_monitored[e_api->events[idx].data.fd].mask;
    // We want to remember the previous events only if they're not of type
    // CLOSE or TIMER
    int mask = ev_mask & (EV_CLOSEFD|EV_TIMERFD) ? ev_mask : EV_NONE;
    if (events & (EPOLLERR | EPOLLHUP | EPOLLRDHUP)) mask |= EV_DISCONNECT;
    if (events & EPOLLIN) mask |= EV_READ;
    if (events & EPOLLOUT) mask |= EV_WRITE;
    return mask;
}

static int ev_api_poll(ev_context *ctx, time_t timeout) {
    struct epoll_api *e_api = ctx->api;
    return epoll_wait(e_api->fd, e_api->events, ctx->events_nr, timeout);
}

static int ev_api_watch_fd(ev_context *ctx, int fd) {
    struct epoll_api *e_api = ctx->api;
    return epoll_add(e_api->fd, fd, EPOLLIN, NULL);
}

static int ev_api_del_fd(ev_context *ctx, int fd) {
    struct epoll_api *e_api = ctx->api;
    return epoll_del(e_api->fd, fd);
}

static int ev_api_register_event(ev_context *ctx, int fd, int mask) {
    struct epoll_api *e_api = ctx->api;
    int op = 0;
    if (mask & EV_READ) op |= EPOLLIN;
    if (mask & EV_WRITE) op |= EPOLLOUT;
    return epoll_add(e_api->fd, fd, op, NULL);
}

static int ev_api_fire_event(ev_context *ctx, int fd, int mask) {
    struct epoll_api *e_api = ctx->api;
    int op = 0;
    if (mask & EV_READ) op |= EPOLLIN;
    if (mask & EV_WRITE) op |= EPOLLOUT;
    if (mask & EV_EVENTFD)
        return epoll_add(e_api->fd, fd, op, NULL);
    return epoll_mod(e_api->fd, fd, op, NULL);
}

/*
 * Get the event on the idx position inside the events map. The event can also
 * be an unset one (EV_NONE)
 */
static inline struct ev *ev_api_fetch_event(const ev_context *ctx,
                                            int idx, int mask) {
    (void) mask; // silence the compiler warning
    int fd = ((struct epoll_api *) ctx->api)->events[idx].data.fd;
    return ctx->events_monitored + fd;
}

#elif defined(POLL)


#elif defined(SELECT)


#elif defined(KQUEUE)



#endif // KQUEUE

static ev_context ev_default_ctx;
static int ev_default_ctx_inited = 0;
#ifdef __linux__
static int quit_sig;
#else
static int quit_sig[2];
#endif

// Stops epoll_wait loops by sending an event
static void ev_sigint_handler(int signum) {
    (void) signum;
#ifdef __linux__
    eventfd_write(quit_sig, 1);
#else
    (void) write(quit_sig[0], &(unsigned long) {1}, sizeof(unsigned long));
#endif
}

/*
 * Eventloop stop callback, will be triggered by an EV_CLOSEFD event and stop
 * the running loop, unblocking the call.
 */
static void ev_stop_callback(ev_context *ctx, void *arg) {
    (void) arg;
    ev_stop(ctx);
}

/*
 * Process the event at the position idx in the events_monitored array. Read or
 * write events can be executed on the same iteration, differentiating just
 * on EV_CLOSEFD or EV_EVENTFD.
 * Returns the number of fired callbacks.
 */
static int ev_process_event(ev_context *ctx, int idx, int mask) {
    if (mask == EV_NONE) return EV_OK;
    struct ev *e = ev_api_fetch_event(ctx, idx, mask);
    int err = 0, fired = 0, fd = e->fd;
    if (mask & EV_CLOSEFD) {
#ifdef __linux__
        err = eventfd_read(fd, &(eventfd_t){0});
#else
        err = read(fd, &(unsigned long){0}, sizeof(unsigned long));
#endif // __linux__
        if (err < 0) return EV_OK;
        e->rcallback(ctx, e->rdata);
        ++fired;
    } else {
        if (mask & EV_EVENTFD) {
#ifdef __linux__
            err = eventfd_read(fd, &(eventfd_t){0L});
#else
            err = read(fd, &(unsigned long){0}, sizeof(unsigned long));
#endif // __linux__
            close(fd);
        } else if (mask & EV_TIMERFD) {
            err = read(fd, &(unsigned long int){0L}, sizeof(unsigned long int));
        }
        if (err < 0) return EV_OK;
        if (mask & EV_READ) {
            e->rcallback(ctx, e->rdata);
            ++fired;
        }
        if (mask & EV_WRITE) {
            if (!fired || e->wcallback != e->rcallback) {
                e->wcallback(ctx, e->wdata);
                ++fired;
            }
        }
    }
    return fired;
}

/*
 * Auxiliary function, update FD, mask and data in monitored events array.
 * Monitored events are the same number as the maximum FD registered in the
 * context.
 */
static void ev_add_monitored(ev_context *ctx, int fd, int mask,
                             void (*callback)(ev_context *, void *),
                             void *ptr) {
    /*
     * TODO check for fd <= 1024 if using SELECT
     * That is because FD_SETSIZE is fixed to 1024, fd_set is an array of 32
     * i32 and each FD is represented by a bit so 32 x 32 = 1024 as hard limit
     */
    if (fd >= ctx->maxevents) {
        int i = ctx->maxevents;
        ctx->maxevents = fd;
        if (fd >= ctx->events_nr) {
            ctx->events_monitored =
                realloc(ctx->events_monitored, (fd + 1) * sizeof(struct ev));
            for (; i < ctx->maxevents; ++i)
                ctx->events_monitored[i].mask = EV_NONE;
        }
    }
    ctx->events_monitored[fd].fd = fd;
    ctx->events_monitored[fd].mask |= mask;
    if (mask & EV_READ) {
        ctx->events_monitored[fd].rdata = ptr;
        ctx->events_monitored[fd].rcallback = callback;
    }
    if (mask & EV_WRITE) {
        ctx->events_monitored[fd].wdata = ptr;
        ctx->events_monitored[fd].wcallback = callback;
    }
}

static inline int ev_get_event_type(ev_context *ctx, int idx) {
    return ev_api_get_event_type(ctx, idx);
}

ev_context *ev_get_ev_context(void) {
    if (ev_default_ctx_inited == 0) {
#ifdef __linux__
        quit_sig = eventfd(0, EFD_NONBLOCK);
#else
        pipe(quit_sig);
#endif
        signal(SIGINT, ev_sigint_handler);
        signal(SIGTERM, ev_sigint_handler);
        int err = ev_init(&ev_default_ctx, EVENTLOOP_MAX_EVENTS);
        if (err < EV_OK)
            return NULL;
#ifdef __linux__
        ev_register_event(&ev_default_ctx, quit_sig,
                          EV_CLOSEFD | EV_READ, ev_stop_callback, NULL);
#else
        ev_register_event(&ev_default_ctx, quit_sig[1],
                          EV_CLOSEFD | EV_READ, ev_stop_callback, NULL);
#endif
        ev_default_ctx_inited = 1;
    }
    return &ev_default_ctx;
}

int ev_init(ev_context *ctx, int events_nr) {
    int err = ev_api_init(ctx, events_nr);
    if (err < EV_OK)
        return err;
    ctx->stop = 0;
    ctx->fired_events = 0;
    ctx->is_running = 0;
    ctx->maxevents = events_nr;
    ctx->events_nr = events_nr;
    ctx->events_monitored = calloc(events_nr, sizeof(struct ev));
    return EV_OK;
}

int ev_is_running(const ev_context *ctx) {
    return ctx->is_running;
}

void ev_destroy(ev_context *ctx) {
    for (int i = 0; i < ctx->maxevents; ++i) {
        if (!(ctx->events_monitored[i].mask & EV_CLOSEFD) &&
            ctx->events_monitored[i].mask != EV_NONE)
            ev_del_fd(ctx, ctx->events_monitored[i].fd);
    }
    ctx->is_running = 0;
    free(ctx->events_monitored);
    ev_api_destroy(ctx);
}

/*
 * Poll an event context for events, accepts a timeout or block forever,
 * returning only when a list of FDs are ready to either READ, WRITE or TIMER
 * to be executed.
 */
int ev_poll(ev_context *ctx, time_t timeout) {
    return ev_api_poll(ctx, timeout);
}

/*
 * Blocks forever in a loop polling for events with ev_poll calls. At every
 * cycle executes callbacks registered with each event
 */
int ev_run(ev_context *ctx) {
    int n = 0, events = 0;
    /*
     * Start an infinite loop, can be stopped only by scheduling an ev_stop
     * callback or if an error on the underlying backend occur
     */
    ctx->is_running = 1;
    while (!ctx->stop) {
        /*
         * blocks polling for events, -1 means forever. Returns only in case of
         * valid events ready to be processed or errors
         */
        n = ev_poll(ctx, EVENTLOOP_TIMEOUT);
        if (n < 0) {
            /* Signals to all threads. Ignore it for now */
            if (errno == EINTR)
                continue;
            /* Error occured, break the loop */
            break;
        }
        for (int i = 0; i < n; ++i) {
            events = ev_get_event_type(ctx, i);
            ctx->fired_events += ev_process_event(ctx, i, events);
        }
    }
    return n;
}

/*
 * Trigger a stop on a running event, it's meant to be run as an event in a
 * running ev_ctx
 */
void ev_stop(ev_context *ctx) {
    ctx->is_running = 0;
    ctx->stop = 1;
}

/*
 * Add a single FD to the underlying backend of the event loop. Equal to
 * ev_fire_event just without an event to be carried. Useful to add simple
 * descritors like a listening socket o message queue FD.
 */
int ev_watch_fd(ev_context *ctx, int fd, int mask) {
    ev_add_monitored(ctx, fd, mask, NULL, NULL);
    return ev_api_watch_fd(ctx, fd);
}

/*
 * Remove a FD from the loop, even tho a close syscall is sufficient to remove
 * the FD from the underlying backend such as EPOLL/SELECT, this call ensure
 * that any associated events is cleaned out an set to EV_NONE
 */
int ev_del_fd(ev_context *ctx, int fd) {
    memset(ctx->events_monitored + fd, 0x00, sizeof(struct ev));
    return ev_api_del_fd(ctx, fd);
}

/*
 * Register a new event, semantically it's equal to ev_register_event but
 * it's meant to be used when an FD is not already watched by the event loop.
 * It could be easily integrated in ev_fire_event call but I prefer maintain
 * the samantic separation of responsibilities.
 *
 * Set a callback and an argument to be passed to for the next loop cycle,
 * associating it to a file descriptor, ultimately resulting in an event to be
 * dispatched and processed.
 *
 * The difference with ev_fire_event is that this function should be called
 * when the file descriptor is not registered in the loop yet.
 *
 * - mask: bitmask used to describe what type of event we're going to fire
 * - callback:  is a function pointer to the routine we want to execute
 * - data:  an opaque pointer to the arguments for the callback.
 */
int ev_register_event(ev_context *ctx, int fd, int mask,
                      void (*callback)(ev_context *, void *), void *data) {
    ev_add_monitored(ctx, fd, mask, callback, data);
    int ret = 0;
    ret = ev_api_register_event(ctx, fd, mask);
    if (ret < 0) return EV_ERR;
    if (mask & EV_EVENTFD)
#ifdef __linux__
        (void) eventfd_write(fd, 1);
#else
        (void) write(fd, &(unsigned long){1}, sizeof(unsigned long));
#endif
    return EV_OK;
}

/*
 * Register a periodically repeate callback and args to be passed to a running
 * loop, specifying, seconds and/or nanoseconds defining how often the callback
 * should be executed.
 */
int ev_register_cron(ev_context *ctx,
                     void (*callback)(ev_context *, void *),
                     void *data,
                     long long s, long long ns) {
#ifdef __linux__
    struct itimerspec timer;
    memset(&timer, 0x00, sizeof(timer));
    timer.it_value.tv_sec = s;
    timer.it_value.tv_nsec = ns;
    timer.it_interval.tv_sec = s;
    timer.it_interval.tv_nsec = ns;

    int timerfd = timerfd_create(CLOCK_MONOTONIC, TFD_NONBLOCK | TFD_CLOEXEC);

    if (timerfd_settime(timerfd, 0, &timer, NULL) < 0)
        return EV_ERR;

    // Add the timer to the event loop
    ev_add_monitored(ctx, timerfd, EV_TIMERFD|EV_READ, callback, data);
    return ev_api_watch_fd(ctx, timerfd);
#else
    struct kqueue_api *k_api = ctx->api;
    // milliseconds
    unsigned period = (s * 1000)  + (ns / 100);
    int fd = socket(AF_INET, SOCK_STREAM, 0);
    ev_add_monitored(ctx, fd, EV_TIMERFD|EV_READ, callback, data);
    struct kevent ke;
    EV_SET(&ke, fd, EVFILT_TIMER, EV_ADD | EV_ENABLE, 0, period, 0);
    if (kevent(k_api->fd, &ke, 1, NULL, 0, NULL) == -1)
        return EV_ERR;
    return EV_OK;
#endif // __linux__
}

/*
 * Register a new event for the next loop cycle to a FD. Equal to ev_watch_fd
 * but allow to carry an event object for the next cycle.
 *
 * Set a callback and an argument to be passed to for the next loop cycle,
 * associating it to a file descriptor, ultimately resulting in an event to be
 * dispatched and processed.
 *
 * Behave like ev_register_event but it's meant to be called when the file
 * descriptor is already registered in the loop.
 *
 * - mask: bitmask used to describe what type of event we're going to fire
 * - callback:  is a function pointer to the routine we want to execute
 * - data:  an opaque pointer to the arguments for the callback.
 */
int ev_fire_event(ev_context *ctx, int fd, int mask,
                  void (*callback)(ev_context *, void *), void *data) {
    int ret = 0;
    ev_add_monitored(ctx, fd, mask, callback, data);
    ret = ev_api_fire_event(ctx, fd, mask);
    if (ret < 0) return EV_ERR;
    if (mask & EV_EVENTFD) {
#ifdef __linux__
        ret = eventfd_write(fd, 1);
#else
        ret = write(fd, &(unsigned long){1}, sizeof(unsigned long));
#endif // __linux__
        if (ret < 0) return EV_ERR;
    }
    return EV_OK;
}

#endif // EV_SOURCE_ONCE
#endif // EV_SOURCE

#endif // EV_H
