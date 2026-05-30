#include "common.h"



void *socket_thread(void *arg)
{

}

void *rpc_server_thread(void *arg)
{

}

int main()
{
    printf("=== service 服务端启动 ===\n");
    pthread_t tid_collect;
    pthread_create(&tid_collect, NULL, socket_thread, NULL);


    pthread_t tid_rpc_server;
    pthread_create(&tid_rpc_server, NULL, rpc_server_thread, NULL);
    while(1)
    {
        sleep(1);
    }
    pthread_join(tid_collect,NULL);
    pthread_join(tid_rpc_server,NULL);

}