#include "dht3x.h"
#include "i2c.h"
#include <unistd.h>
#include <stdio.h>
#include <sys/ioctl.h>
#include <linux/i2c-dev.h>

#define DHT3X_ADDR 0x44

int dht3x_init(const char *i2c_dev)
{
    // 初始化 I2C
    int fd = i2c_init(i2c_dev, DHT3X_ADDR);
    if (fd < 0)
        return -1;

    // 这里不需要发命令
    return fd;
}

int dht3x_read(int fd, float *temp, float *hum)
{

    unsigned char buf[6] = {0};

    if(dht3x_read_raw(fd, buf) < 0)
    {
        perror("dht3x read failed");
        return -1;
    }



    // 计算
    *temp = -45.0f + 175.0f * ((buf[0] << 8) | buf[1]) / 65535.0f;
    *hum  = 100.0f  * ((buf[3] << 8) | buf[4]) / 65535.0f;

    return 0;
}