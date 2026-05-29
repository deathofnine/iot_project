#include "sht3x.h"
#include "i2c.h"
#include <unistd.h>
#include <stdio.h>
#include <sys/ioctl.h>
#include <linux/i2c-dev.h>

#define SHT3X_ADDR 0x44

int sht3x_init(const char *i2c_dev)
{
    // 初始化 I2C
    int fd = i2c_init(i2c_dev, SHT3X_ADDR);
    if (fd < 0)
        return -1;
    // 这里不需要发命令
    return fd;
}

int sht3x_read(int fd, float *temp, float *hum)
{

    unsigned char buf[6] = {0};
    //无时钟拉伸
    unsigned char cmd[2] = {0x2C, 0x06};
    if(write(fd,cmd,2)!=2)
    {
        perror("write failed\n");
        return -1;
    }

    if(read(fd,buf,6)!=6)
    {
        perror("read failed\n");
        return -1;
    }


    // 计算
    *temp = -45.0f + 175.0f * ((buf[0] << 8) | buf[1]) / 65535.0f;
    *hum  = 100.0f  * ((buf[3] << 8) | buf[4]) / 65535.0f;

    return 0;
}