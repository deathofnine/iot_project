#include "common.h"
#include "mpu6050.h"
#include "sht3x.h"
#include "pthread.h"
#include <sys/types.h>          /* See NOTES */
#include <sys/socket.h>
#include <sys/un.h>
#include <signal.h>
pthread_mutex_t g_data_mutex;
sensor_data_t g_sensor_data;
int g_fd_sock = -1;
static int running = 1; 

// 信号处理函数
void signal_handler(int sig) {
    if (sig == SIGINT || sig == SIGTERM) {
        printf("\n收到退出信号，正在关闭...\n");
        running = 0;
    }
}

//-----------------------------------
//采集线程
//-----------------------------------
void *collect_pthread(void *arg)
{
    float temp, hum;
    float ax, ay, az, gx, gy, gz;
    // MPU6050初始化
    int fd_mpu = mpu6050_init("/dev/i2c-0");
    if(fd_mpu<=0) printf("MPU6050 init failed\n");


    // SHT3X初始化
    int fd_sht = sht3x_init("/dev/i2c-0");
    if(fd_sht<=0) printf("SHT3X init failed\n");
    printf("采集线程成功启动\n");
    printf("功能：I2C传感器采集\n");
    while(running)
    {
        //MPU读取
        mpu6050_read(fd_mpu, &ax, &ay, &az, &gx, &gy, &gz);
        printf("AX:%.2f AY:%.2f AZ:%.2f | GX:%.2f GY:%.2f GZ:%.2f\n",ax, ay, az, gx, gy, gz);
        //sht读取
        sht3x_read(fd_sht, &temp, &hum);
        printf("TEMP:%.1f HUM:%.1f\n",temp,hum);
        pthread_mutex_lock(&g_data_mutex);

        // 加锁更新共享数据
        g_sensor_data.temp = temp;
        g_sensor_data.hum  = hum;
        g_sensor_data.ax   = ax;
        g_sensor_data.ay   = ay;
        g_sensor_data.az   = az;
        g_sensor_data.gx   = gx;
        g_sensor_data.gy   = gy;
        g_sensor_data.gz   = gz;
        pthread_mutex_unlock(&g_data_mutex);
        sleep(1);
    }
    printf("采集线程关闭\n");
    close(fd_mpu);
    close(fd_sht);
    return NULL;
}   

void *client_pthread(void *arg)
{
    sleep(5);
    while(running)
    {
        // 1. 创建客户端 socket
        int fd_sock = socket(AF_UNIX, SOCK_STREAM, 0);
        if(fd_sock<0)
        {
            printf("socket failed\n");
            sleep(1);
            continue;
        }

        // 2. 连接 service 服务端
        struct sockaddr_un addr;
        memset(&addr, 0, sizeof(addr));
        addr.sun_family=AF_UNIX;
        strcpy(addr.sun_path, SOCKET_PATH);
        if(connect(fd_sock,(const struct sockaddr *)&addr,sizeof(addr))<0)
        {
            printf("connect failed\n");
            sleep(1);
            continue;
        }
        // 3. 加锁读取数据
        pthread_mutex_lock(&g_data_mutex);
        sensor_data_t data = g_sensor_data;
        pthread_mutex_unlock(&g_data_mutex);
        // 4. 拼接成 JSON
        char json[256];
        snprintf(json, sizeof(json),
        "{\"temp\":%.2f,\"hum\":%.2f,\"ax\":%.2f,\"ay\":%.2f,\"az\":%.2f,\"gx\":%.2f,\"gy\":%.2f,\"gz\":%.2f}",
        data.temp, data.hum, data.ax, data.ay, data.az, data.gx, data.gy, data.gz);
        write(fd_sock, json, strlen(json));
        printf("上报成功：%s\n", json);

        close(fd_sock);
        sleep(1);
    }
    printf("socket 关闭\n");
    return NULL;
} 




int main()
{

    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);
    // 初始化互斥锁
    pthread_mutex_init(&g_data_mutex, NULL);
    // 1. 启动传感器采集线程
    pthread_t collect_tid;
    if(pthread_create(&collect_tid,NULL,collect_pthread,NULL)!=0)
    {
        perror("collect_pthread create failed");
        return -1;
    }
    // 2.上报数据给后台
    pthread_t client_tid;
    if(pthread_create(&client_tid,NULL,client_pthread,NULL)!=0)
    {
        perror("collect_pthread create failed");
        return -1;
    }


    printf("采集进程成功启动");
    printf("功能：I2C 传感器采集 + 本地socket发送数据\n");

    pthread_join(collect_tid,NULL);
    pthread_join(client_tid,NULL);
    pthread_mutex_destroy(&g_data_mutex);
    printf("程序正常退出\n");
    return 0;
}