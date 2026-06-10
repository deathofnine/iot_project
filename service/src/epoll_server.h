#ifndef EPOLL_SERVER_H
#define EPOLL_SERVER_H

#include "common.h"
#include <pthread.h>

// 对外导出epoll主线程函数
void *epoll_io_thread(void *arg);

#endif