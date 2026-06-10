#include "common.h"
#include "socket_server.h"
#include "rpc_server.h"
#include "service.h"
#include "mqtt_upload.h"
#include "epoll_server.h"
sensor_data_t g_sensor;
pthread_mutex_t g_data_mutex;


int main()
{


    
    pthread_mutex_init(&g_data_mutex, NULL);
    printf("=== service 服务端启动 ===\n");
    pthread_t tid_epoll;
    pthread_create(&tid_epoll, NULL, epoll_io_thread, NULL);

    pthread_t tid_mqtt;
    pthread_create(&tid_mqtt, NULL, mqtt_upload_thread, NULL);
    while(1)
    {
        sleep(1);
    }
    pthread_join(tid_epoll,NULL);

    pthread_mutex_destroy(&g_data_mutex);

}