#include<stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <linux/i2c-dev.h>

// I2C 初始化
int i2c_init(const char *dev_path,unsigned char dev_addr)
{
    int fd = open(dev_path,O_RDWR);
    if(fd < 0)
    {
        perror("i2c open failed");
        return -1;
    }
    if(ioctl(fd,I2C_SLAVE,dev_addr)<0)
    {
        perror("i2c set addr failed");
        close(fd);
        return -1;
    }
    return fd;
}

// I2C 读寄存器
int i2c_read_reg(int fd, unsigned char reg, unsigned char *buf, int len)
{
    if(write(fd,&reg,1)!=1)
    {
        perror("write reg failed");
        return -1;
    }
    if (read(fd, buf, len) != len) {
        perror("read data failed");
        return -1;
    }
    return len;
}
// I2C 写寄存器 
int i2c_write_reg(int fd, unsigned char reg, unsigned char val)
{   
    unsigned char buf[2] = {reg, val};
    if (write(fd, buf, 2) != 2) {
        perror("write failed");
        return -1;
    }
    return 0;

}