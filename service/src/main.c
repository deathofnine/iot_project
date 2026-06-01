#include "common.h"
#include "socket_server.h"
#include "rpc_server.h"
#include "service.h"
#include "mqtt_upload.h"
sensor_data_t g_sensor;
pthread_mutex_t g_data_mutex;


int main()
{


    
    pthread_mutex_init(&g_data_mutex, NULL);
    printf("=== service 服务端启动 ===\n");
    pthread_t tid_collect;
    pthread_create(&tid_collect, NULL, socket_server_thread, NULL);
    

    pthread_t tid_rpc_server;
    pthread_create(&tid_rpc_server, NULL, rpc_server_thread, NULL);

    pthread_t tid_mqtt;
    pthread_create(&tid_mqtt, NULL, mqtt_upload_thread, NULL);
    while(1)
    {
        sleep(1);
    }
    pthread_join(tid_collect,NULL);
    pthread_join(tid_rpc_server,NULL);
    pthread_mutex_destroy(&g_data_mutex);

}