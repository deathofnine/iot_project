#ifndef __MPU6050_H__
#define __MPU6050_H__

#include "common.h"

// MPU6050 地址
#define MPU6050_ADDR 0x68

// 初始化 MPU6050
int mpu6050_init(const char *i2c_dev);

// 读取数据
int mpu6050_read(int fd, float *ax, float *ay, float *az,
                 float *gx, float *gy, float *gz);

#endif