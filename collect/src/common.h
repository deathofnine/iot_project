#ifndef __COMMON_H__
#define __COMMON_H__
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <stdint.h>

// 传感器数据结构体（三进程统一用这个）
typedef struct {
    float temp;       // 温度
    float hum;        // 湿度
    float ax, ay, az; // MPU6050 加速度
    float gx, gy, gz; // MPU6050 陀螺仪
    int led;          // LED 状态 0/1
} sensor_data_t;

// 本地Socket通信路径
#define SOCKET_PATH "/tmp/iot_collect_socket"

// 指令类型
#define CMD_GET_DATA 1
#define CMD_SET_LED  2

#endif