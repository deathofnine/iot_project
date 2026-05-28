#include "common.h"
#include "i2c.h"
int main()
{
    printf("采集进程成功启动");
    printf("功能：I2C 传感器采集 + 本地socket服务\n");
    int fd = i2c_init("/dev/i2c-0",0x44);
    if(fd<=0)
    {
        printf("i2c init failed\n");
        return -1;
    }
    printf("i2c init success, fd=%d\n", fd);
    while(1)
    {
        sleep(1);
    }
    return 0;
}