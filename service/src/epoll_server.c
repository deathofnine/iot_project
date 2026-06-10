#include "common.h"
#include <sys/epoll.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <errno.h>
#include <cJSON.h>

#define EPOLL_MAX_EVENTS 16

// fd类型标记
typedef enum {
    FD_LISTEN_SENSOR,   // /tmp/sensor.sock 监听fd
    FD_LISTEN_RPC,      // /tmp/rpc.sock 监听fd
    FD_CLIENT_SENSOR,   // 采集进程客户端
    FD_CLIENT_RPC       // RPC控制客户端
} fd_type_e;

// epoll附加数据，区分fd用途
typedef struct {
    int fd;
    fd_type_e type;
} epoll_ctx_t;

extern sensor_data_t g_sensor;
extern pthread_mutex_t g_data_mutex;

static int epoll_add(int epfd, int fd, uint32_t events, fd_type_e t)
{
    epoll_ctx_t *ctx = malloc(sizeof(epoll_ctx_t));
    ctx->fd = fd;
    ctx->type = t;

    struct epoll_event ev;
    ev.data.ptr = ctx;
    ev.events = events;
    if (epoll_ctl(epfd, EPOLL_CTL_ADD, fd, &ev) < 0) {
        free(ctx);
        perror("epoll_ctl add");
        return -1;
    }
    return 0;
}

static void epoll_del(int epfd, int fd, epoll_ctx_t *ctx)
{
    epoll_ctl(epfd, EPOLL_CTL_DEL, fd, NULL);
    free(ctx);
    close(fd);
}

// 创建传感器监听socket
static int create_sensor_listen(void)
{
    int fd = socket(AF_UNIX, SOCK_STREAM, 0);
    unlink(SOCKET_PATH);
    struct sockaddr_un addr;
    memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_UNIX;
    strcpy(addr.sun_path, SOCKET_PATH);
    bind(fd, (struct sockaddr *)&addr, sizeof(addr));
    listen(fd, 5);
    printf("[epoll] sensor listen create ok %s\n", SOCKET_PATH);
    return fd;
}

// 创建RPC监听socket
static int create_rpc_listen(void)
{
    int fd = socket(AF_UNIX, SOCK_STREAM, 0);
    unlink(RPC_PATH);
    struct sockaddr_un addr;
    memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_UNIX;
    strcpy(addr.sun_path, RPC_PATH);
    bind(fd, (struct sockaddr *)&addr, sizeof(addr));
    listen(fd, 5);
    printf("[epoll] rpc listen create ok %s\n", RPC_PATH);
    return fd;
}

// 处理采集端推送的传感器JSON
static void handle_sensor_client(int cli_fd, char *buf, int len)
{
    buf[len] = '\0';
    cJSON *root = cJSON_Parse(buf);
    if (!root) return;

    pthread_mutex_lock(&g_data_mutex);
    g_sensor.temp = cJSON_GetObjectItem(root, "temp")->valuedouble;
    g_sensor.hum  = cJSON_GetObjectItem(root, "hum")->valuedouble;
    g_sensor.ax   = cJSON_GetObjectItem(root, "ax")->valuedouble;
    g_sensor.ay   = cJSON_GetObjectItem(root, "ay")->valuedouble;
    g_sensor.az   = cJSON_GetObjectItem(root, "az")->valuedouble;
    g_sensor.gx   = cJSON_GetObjectItem(root, "gx")->valuedouble;
    g_sensor.gy   = cJSON_GetObjectItem(root, "gy")->valuedouble;
    g_sensor.gz   = cJSON_GetObjectItem(root, "gz")->valuedouble;
    pthread_mutex_unlock(&g_data_mutex);

    printf("[epoll] sensor data update\n");
    cJSON_Delete(root);
}

// 处理RPC控制指令
static void handle_rpc_client(int cli_fd, char *buf, int len)
{
    buf[len] = '\0';
    int cmd = atoi(buf);
    char json[256] = {0};

    pthread_mutex_lock(&g_data_mutex);
    sensor_data_t data = g_sensor;
    pthread_mutex_unlock(&g_data_mutex);

    switch (cmd)
    {
        case CMD_GET_DATA:
            snprintf(json, sizeof(json),
                "{"
                    "\"temp\":%.2f,"
                    "\"hum\":%.2f,"
                    "\"ax\":%.2f,"
                    "\"ay\":%.2f,"
                    "\"az\":%.2f,"
                    "\"gx\":%.2f,"
                    "\"gy\":%.2f,"
                    "\"gz\":%.2f"
                "}",
                data.temp, data.hum,
                data.ax, data.ay, data.az,
                data.gx, data.gy, data.gz);
            break;
        case CMD_SET_LED:
            snprintf(json, sizeof(json), "{\"result\":\"LED set ok\"}");
            break;
        default:
            snprintf(json, sizeof(json), "{\"result\":\"unknown cmd\"}");
            break;
    }
    write(cli_fd, json, strlen(json));
}

void *epoll_io_thread(void *arg)
{
    int epfd = epoll_create1(0);
    int fd_sensor_listen = create_sensor_listen();
    int fd_rpc_listen = create_rpc_listen();

    epoll_add(epfd, fd_sensor_listen, EPOLLIN, FD_LISTEN_SENSOR);
    epoll_add(epfd, fd_rpc_listen, EPOLLIN, FD_LISTEN_RPC);

    struct epoll_event events[EPOLL_MAX_EVENTS];
    char buf[256];

    while (1)
    {
        int n = epoll_wait(epfd, events, EPOLL_MAX_EVENTS, 100);
        if (n <= 0) continue;

        for (int i = 0; i < n; i++)
        {
            epoll_ctx_t *ctx = events[i].data.ptr;
            int fd = ctx->fd;

            // 1. 传感器socket有新连接
            if (ctx->type == FD_LISTEN_SENSOR)
            {
                int cli = accept(fd, NULL, NULL);
                epoll_add(epfd, cli, EPOLLIN, FD_CLIENT_SENSOR);
                continue;
            }
            // 2. RPC socket有新连接
            if (ctx->type == FD_LISTEN_RPC)
            {
                int cli = accept(fd, NULL, NULL);
                epoll_add(epfd, cli, EPOLLIN, FD_CLIENT_RPC);
                continue;
            }

            // 3. 客户端读事件
            int r = read(fd, buf, sizeof(buf)-1);
            if (r <= 0)
            {
                epoll_del(epfd, fd, ctx);
                continue;
            }

            if (ctx->type == FD_CLIENT_SENSOR)
            {
                handle_sensor_client(fd, buf, r);
            }
            else if (ctx->type == FD_CLIENT_RPC)
            {
                handle_rpc_client(fd, buf, r);
            }
            epoll_del(epfd, fd, ctx); // 短连接，读完直接关闭
        }
    }

    close(fd_sensor_listen);
    close(fd_rpc_listen);
    close(epfd);
    return NULL;
}