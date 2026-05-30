#include "common.h"
#include <sys/socket.h>
#include <sys/un.h>
#include <errno.h>
void *socket_server_thread(void *arg)
{   
    int fd_server, fd_client;
    struct sockaddr_un addr;
    char buf[256];
    unlink(SOCKET_PATH);

    fd_server= socket(AF_UNIX,SOCK_STREAM,0);
    if(fd_server<0)
    {
        perror("socket failed");
        return NULL;
    }

    memset (&addr,0,sizeof(addr));
    addr.sun_family = AF_UNIX;
    strcpy(addr.sun_path,SOCKET_PATH);
    if(bind(fd_server,(struct sockaddr *)&addr,sizeof(addr))< 0)
    {
        perror("bind failed");
        close(fd_server);
        return NULL;
    }
     if (listen(fd_server, 5) < 0)
     {
        perror("listen failed");
        close(fd_server);
        return NULL;
     }
     printf("[service] socket 服务端启动成功：%s\n", SOCKET_PATH);
     while (1)
     {
        fd_client = accept(fd_server, NULL, NULL);
        if (fd_client < 0) {
            usleep(100000);
            continue;
        }
        int rev_len=read(fd_client,buf,sizeof(buf)-1);
        if(rev_len<=0)
        {
            close (fd_client);
            continue;
        }
        buf[rev_len]='\0';
        // 解析 JSON（严格按你的结构体）
        cJSON *root = cJSON_Parse(buf);
        if (root) 
        {
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
            printf("[service] 数据更新成功！\n");
            cJSON_Delete(root);
            close(fd_client);
        }

     }
     close(fd_server);
     
}