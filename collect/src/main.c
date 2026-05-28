#include "common.h"
#include "mpu6050.h"
#include "dht3x.h"
int main()
{
    printf("采集进程成功启动");
    printf("功能：I2C 传感器采集 + 本地socket服务\n");

    // MPU6050
    int fd_mpu = mpu6050_init("/dev/i2c-0");
    if(fd_mpu<=0) printf("MPU6050 init failed\n");

    // DHT3X
    int fd_dht = mpu6050_init("/dev/i2c-0");
    if(fd_dht<=0) printf("DHT3X init failed\n");

    float temp, hum;
    float ax, ay, az, gx, gy, gz;
    while(1)
    {
        printf("fd_mpu=%d ,fd_dht=%d\n",fd_mpu,fd_dht);
        dht3x_read(fd_dht, &temp, &hum);
        printf("TEMP:%.1f HUM:%.1f\n",temp,hum);
        mpu6050_read(fd_mpu, &ax, &ay, &az, &gx, &gy, &gz);
        printf("AX:%.2f AY:%.2f AZ:%.2f | GX:%.2f GY:%.2f GZ:%.2f\n",ax, ay, az, gx, gy, gz);
        
        sleep(1);
    }
    close(fd_mpu);
    return 0;
}