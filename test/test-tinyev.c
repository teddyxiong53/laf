// #include "tinyev.h"
#include <stdio.h>
#include <netdb.h>
#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#if 0  //用原版测试
#define EV_SOURCE // add before ev.h
#include "ev.h"
#else // 使用我的
#include "tinyev.h"
#endif
#include "mylog.h"

#define PORT 2345
#define BUFSIZE 1024
static void on_response(ev_context *ctx, void *data);
struct connection {
    int fd;
    int bufsize;
    int capacity;
    char *buf;
};
static void on_data (ev_context *ctx, void *data);

static inline int make_nonblocking(int fd) {
    int flags, result;
    flags = fcntl(fd, F_GETFL, 0);

    if (flags == -1)
        goto err;

    result = fcntl(fd, F_SETFL, flags | O_NONBLOCK);
    if (result == -1)
        goto err;

    return 0;

err:

    myloge( "make_nonblocking: %s", strerror(errno));
    return -1;
}

static int connection_init(struct connection *conn, int fd)
{
    if (!conn) {
        return -1;
    }
    conn->fd = fd;
    mylogd("conn->fd:%d", fd);
    conn->bufsize = 0;
    conn->capacity = BUFSIZE;
    conn->buf = calloc(1, BUFSIZE);
    return 0;
}
static void connection_close(struct connection *conn)
{
    if (!conn) {
        return;
    }
    close(conn->fd);
    free(conn->buf);
    free(conn);
}

static void on_connection(ev_context *ctx, void *data)
{
    int listen_fd = *((int *)data);
    int ret = 0;
    mylogd("listen fd:%d", listen_fd);
    while (1) {
        int fd = 0;
        struct sockaddr_in addr;
        int flags = SOCK_NONBLOCK;
        socklen_t addrlen = sizeof(addr);
        fd = accept(listen_fd, (struct sockaddr *)&addr, &addrlen);
        if (fd < 0) {
            myloge("accept fail, reason:%s, now break accept loop", strerror(errno));
            break;
        }
        make_nonblocking(fd);
        mylogd("get connection fd:%d", fd);
        if (fd == 0) {//什么时候会这样？
            mylogd("return fd == 0");
            continue;
        }
        struct connection *conn = malloc(sizeof(*conn));
        ret = connection_init(conn, fd);
        if (ret < 0) {
            myloge("malloc fail");
            exit(-1);
        }

        ev_register_event(ctx, fd, EV_READ, on_data, conn);

    }
}

static void on_data (ev_context *ctx, void *data)
{
    int n = 0;
    struct connection *conn = data;


    do {

        n = read(conn->fd, conn->buf + conn->bufsize, conn->capacity - conn->bufsize);
        mylogd("read data len:%d", n);
        if (n < 0) {
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                continue;
            } else {
                myloge("fatal err happens, reason:%s", strerror(errno));
                goto err;
            }
        }
        conn->bufsize += n;
        if (conn->bufsize == conn->capacity) {
            conn->capacity *= 2;
            conn->buf = realloc(conn->buf, conn->capacity);
            if (!conn->buf) {
                myloge("malloc fail");
                goto err;
            }
        }
    } while (n > 0);
    if (n == 0) {
        //说明client已经断开了连接
        mylogd("client close connection, so we close it too");
        ev_del_fd(ctx, conn->fd);
        connection_close(conn);
        return;
    }
    if (strncmp((char *)conn->buf, "quit", 4) == 0) {
        mylogd("client send 'quit' to me, so close the connection");
        ev_del_fd(ctx, conn->fd);
        connection_close(conn);
        return;
    }
    mylogd("recevie %d bytes", conn->bufsize);
    ev_fire_event(ctx, conn->fd, EV_WRITE, on_response, conn);

    return ;
err:
    myloge("error reading");
}
static void on_response(ev_context *ctx, void *data)
{
    struct connection *conn = data;
    int n = 0;
    while (conn->bufsize > 0) {
        n = write(conn->fd, conn->buf+n, conn->bufsize);
        if (n < 0) {
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                mylogd("get EAGAIN or EWOULDBLOCK");
                break;
            } else {
                goto err;
            }
        }
        conn->bufsize -= n;
    }
    ev_fire_event(ctx, conn->fd, EV_READ, on_data, conn);
    return;
err:
    myloge("fatal err, %s", strerror(errno));
}


int main(int argc, char const *argv[])
{
    ev_context ctx;
    int sockfd = -1;
    int ret = -1;
    struct sockaddr_in sockaddr = {0};
    sockaddr.sin_family = AF_INET;
    sockaddr.sin_port = htons(PORT);
    sockaddr.sin_addr.s_addr = htonl(INADDR_ANY);

    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        myloge("create socket fail");
        return -1;
    }
    int yes = 1;
    ret = setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR,&yes, sizeof(yes) );
    if (ret < 0) {
        myloge("setsockopt fail");
        goto fail;
    }
    make_nonblocking(sockfd);
    ret = bind(sockfd, (struct sockaddr *)&sockaddr, sizeof(sockaddr));
    if (ret < 0) {
        myloge("bind fail");
        goto fail;
    }

    ret = listen(sockfd, 32);
    if (ret < 0) {
        myloge("listen fail");
        goto fail;
    }

    ret = ev_init(&ctx, 32);
    if (ret < 0) {
        myloge("ev_init fail");
        goto fail;
    }
    mylogd("listen fd:%d", sockfd);
    ev_register_event(&ctx, sockfd, EV_READ, on_connection, &sockfd);
    mylogd("listen on port:%d", PORT);

    ev_run(&ctx);
    ev_destroy(&ctx);
    return 0;
fail:
    return -1;
}
