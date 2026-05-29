#ifndef __I2C_H__
#define __I2C_H__

#include "common.h"

// I2C 初始化
int i2c_init(const char *dev_path,unsigned char dev_addr);

// I2C 读寄存器
int i2c_read_reg(int fd, unsigned char reg, unsigned char *buf, int len);

// I2C 写寄存器
int i2c_write_reg(int fd, unsigned char reg, unsigned char val);


int sht3x_read_raw(int fd, unsigned char *buf);
#endif