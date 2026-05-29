#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <linux/i2c-dev.h>
#include <linux/i2c.h> 
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

int sht3x_read_raw(int fd, unsigned char *buf)
{
    // 必须用 ioctl 直接发 I2C 时序 才能带 RESTART
    struct i2c_msg msgs[2];
    struct i2c_rdwr_ioctl_data data;

    // 第1步：发送 2 字节命令 0x2C 0x06
    unsigned char cmd[2] = {0x2C, 0x06};
    msgs[0].addr  = 0x44;
    msgs[0].flags = 0;       // 写
    msgs[0].len   = 2;
    msgs[0].buf   = cmd;

    // 第2步：读 6 字节（自动RESTART）
    msgs[1].addr  = 0x44;
    msgs[1].flags = 1;       // 读
    msgs[1].len   = 6;
    msgs[1].buf   = buf;

    data.msgs  = msgs;
    data.nmsgs = 2;

    // 直接操作硬件时序
    return ioctl(fd, I2C_RDWR, &data);
}