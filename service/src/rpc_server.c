#include "common.h"
#include <sys/socket.h>
#include <sys/un.h>
#include <errno.h>
void *rpc_server_thread(void *arg)
{
    int fd_server,fd_client;
    struct sockaddr_un addr;

    char buf[256];
    unlink(RPC_PATH);
    fd_server=socket(AF_UNIX,SOCK_STREAM,0);
    if(fd_server<0)
    {
        perror("rpc_socket_failed");
        return NULL;
    }

    memset (&addr,0,sizeof(addr));
    addr.sun_family=AF_UNIX;
    strcpy(addr.sun_path, RPC_PATH);
    if(bind(fd_server,(const struct sockaddr *)&addr,sizeof(addr))<0)
    {
        perror("rpc_bind_failed");
        close(fd_server);
        return NULL;
    }

    if (listen(fd_server, 5) < 0)
    {
    perror("listen failed");
    close(fd_server);
    return NULL;
    }  
    printf("[RPC] 服务已启动：%s\n", RPC_PATH);
    
    while(1)
    {
        fd_client=accept(fd_server,NULL,NULL);
        if(fd_client<0)
        {
            usleep(100000);
            continue;
        } 
        int recv_len=read(fd_client,buf,sizeof(buf)-1);
        if(recv_len<=0)
        {
            close (fd_client);
            continue;
        }
        buf[recv_len]='\0';

        int cmd = atoi(buf);
        char json[256] = {0};

        pthread_mutex_lock(&g_data_mutex);
        sensor_data_t data = g_sensor;
        pthread_mutex_unlock(&g_data_mutex);    

        switch(cmd)
        {

            case CMD_GET_DATA:
                // 返回传感器数据
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
                // 这里可以做LED控制
                snprintf(json, sizeof(json), "{\"result\":\"LED set ok\"}");
                break;

        }
        write(fd_client, json, strlen(json));
        close(fd_client);
    }
    close(fd_server);
    unlink(RPC_PATH);
    return NULL;


}