#include "common.h"
#include <sys/socket.h>
#include <sys/un.h>
#include <errno.h>

void *rpc_server_thread(void *arg)
{
    while(1)
    {
        sleep(1);
    }



}