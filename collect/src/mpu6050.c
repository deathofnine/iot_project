#include "mpu6050.h"
#include "i2c.h"

// MPU6050 寄存器定义
#define PWR_MGMT_1 0x6B
#define ACCEL_XOUT_H 0x3B
#define GYRO_XOUT_H 0x43

int mpu6050_init(const char *i2c_dev)
{
    int fd = i2c_init(i2c_dev, MPU6050_ADDR);
    if (fd < 0) return -1;

    // 唤醒 MPU6050
    if (i2c_write_reg(fd, PWR_MGMT_1, 0x00) < 0) {
        close(fd);
        return -1;
    }

    return fd;
}

int mpu6050_read(int fd, float *ax, float *ay, float *az,
                 float *gx, float *gy, float *gz)
{
    unsigned char buf[14];
    if (i2c_read_reg(fd, ACCEL_XOUT_H, buf, 14) < 0)
        return -1;

    // 原始数据转换
    int16_t ax_raw = (buf[0] << 8) | buf[1];
    int16_t ay_raw = (buf[2] << 8) | buf[3];
    int16_t az_raw = (buf[4] << 8) | buf[5];

    int16_t gx_raw = (buf[8] << 8) | buf[9];
    int16_t gy_raw = (buf[10] << 8) | buf[11];
    int16_t gz_raw = (buf[12] << 8) | buf[13];

    // 转换成物理单位
    *ax = ax_raw / 16384.0f;
    *ay = ay_raw / 16384.0f;
    *az = az_raw / 16384.0f;

    *gx = gx_raw / 131.0f;
    *gy = gy_raw / 131.0f;
    *gz = gz_raw / 131.0f;

    return 0;
}