#include "common.h"
#include "mpu6050.h"
#include "sht3x.h"
#include "pthread.h"
#include "log.h"
#include <signal.h>
#include <fcntl.h>
#include <syslog.h>


pthread_mutex_t g_data_mutex;
sensor_data_t g_sensor_data;

int g_fd_sock = -1;
static int running = 1; 

// 信号处理函数
void signal_handler(int sig) {
    (void)sig;
    running = 0;
}

//-----------------------------------
//守护进程
//-----------------------------------
static int daemonize(void)
{
    pid_t pid = fork();
    if(pid<0)
    {
         LOG_ERROR_ERRNO("fork failed");
        return -1;
    }
    if(pid>0)
    {
        exit(0);
    }
    // 子进程：脱离会话
    setsid();
    umask(0);

    pid=fork();
    if (pid < 0)
    {
        LOG_ERROR_ERRNO("fork2 failed");
        exit(EXIT_FAILURE);
    }
    if (pid > 0)
    {
        exit(0);
    }
    // 修改工作目录到根目录
    chdir("/");
    // 关闭标准输入输出错误，重定向到空设备
    int fd_null = open("/dev/null", O_RDWR);
    if (fd_null >= 0)
    {
        dup2(fd_null, STDIN_FILENO);
        dup2(fd_null, STDOUT_FILENO);
        dup2(fd_null, STDERR_FILENO);
        close(fd_null);
    }
    return 0;
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
    if(fd_mpu<=0){
        LOG_ERROR("MPU6050 init failed");
    }
    
    // SHT3X初始化
    int fd_sht = sht3x_init("/dev/i2c-0");
    if(fd_sht <= 0) {
        LOG_ERROR("SHT3X init failed");
    }
    LOG_COLLECT_INFO("采集线程成功启动");
    LOG_COLLECT_INFO("功能：I2C传感器采集");
    while(running)
    {
        //MPU读取
        mpu6050_read(fd_mpu, &ax, &ay, &az, &gx, &gy, &gz);
        LOG_COLLECT_DEBUG("AX:%.2f AY:%.2f AZ:%.2f | GX:%.2f GY:%.2f GZ:%.2f", ax, ay, az, gx, gy, gz);
        //sht读取
        sht3x_read(fd_sht, &temp, &hum);
        LOG_COLLECT_DEBUG("TEMP:%.1f HUM:%.1f", temp, hum);
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
    LOG_COLLECT_INFO("采集线程关闭");
    close(fd_mpu);
    close(fd_sht);
    return NULL;
}   

//-----------------------------------
//采集线程socket发送
//-----------------------------------
void *client_pthread(void *arg)
{
    sleep(5);
    int fd_sock = -1;

            // service 服务端
            struct sockaddr_un addr;
            memset(&addr, 0, sizeof(addr));
            addr.sun_family=AF_UNIX;
            strcpy(addr.sun_path, SOCKET_PATH);

    while(running)
    {
        // 1. 创建客户端 socket
        if (fd_sock<0)
        {
            fd_sock = socket(AF_UNIX, SOCK_STREAM, 0);
            if(fd_sock<0)
            {
                LOG_ERROR_ERRNO("connect failed");
                sleep(1);
                continue;
            }
            if(connect(fd_sock,(const struct sockaddr *)&addr,sizeof(addr))<0)
            {
                LOG_ERROR_ERRNO("connect failed");
                close(fd_sock);  //关闭fd，重新判断fd_socket
                fd_sock = -1;  
                sleep(1);
                continue;
            }
            LOG_SOCKET_INFO("connect succeed");
        }
        // 3. 加锁读取数据
        pthread_mutex_lock(&g_data_mutex);
        sensor_data_t data = g_sensor_data;
        pthread_mutex_unlock(&g_data_mutex);
        // 4. 拼接成 JSON
        char json[256];
        int ret_len=snprintf(json, sizeof(json),
        "{\"temp\":%.2f,\"hum\":%.2f,\"ax\":%.2f,\"ay\":%.2f,\"az\":%.2f,\"gx\":%.2f,\"gy\":%.2f,\"gz\":%.2f}",
        data.temp, data.hum, data.ax, data.ay, data.az, data.gx, data.gy, data.gz);
        if(ret_len >= (int)sizeof(json)){
        LOG_SOCKET_WARN("JSON缓冲区溢出截断，数据不完整");
        }

        ssize_t ret=write(fd_sock, json, strlen(json));
        if(ret < 0) {
            // 写入失败，说明连接已断开
            LOG_SOCKET_DEBUG("write failed, reconnecting...");
            close(fd_sock);
            fd_sock = -1;           // 标记为断开，下次循环重连
            LOG_SOCKET_DEBUG("连接断开，现在开始重新连接");
            sleep(1);
            continue;
        }
        LOG_SOCKET_DEBUG("json上报成功");
        sleep(1);
    }
    if(fd_sock >= 0) close(fd_sock);
    LOG_SOCKET_INFO("socket 关闭");
    return NULL;
} 




int main()
{
    

    if (daemonize() != 0)
    {
        openlog("collect_daemon", LOG_PID, LOG_DAEMON);
        syslog(LOG_ERR, "daemonize create failed");
        closelog();
        return -1;
    }
    log_init();
    signal(SIGPIPE, SIG_IGN);
    signal(SIGTERM, signal_handler);
    // 初始化互斥锁
    pthread_mutex_init(&g_data_mutex, NULL);
    // 1. 启动传感器采集线程
    pthread_t collect_tid;
    if(pthread_create(&collect_tid,NULL,collect_pthread,NULL)!=0)
    {
        LOG_ERROR_ERRNO("collect_pthread create failed");
        LOG_ERROR("connect failed");
        return -1;
    }
    // 2.上报数据给后台
    pthread_t client_tid;
    if(pthread_create(&client_tid,NULL,client_pthread,NULL)!=0)
    {
        LOG_ERROR_ERRNO("client_pthread create failed");
        return -1;
    }


    LOG_COLLECT_INFO("采集进程成功启动");
    LOG_COLLECT_INFO("功能：I2C 传感器采集 + 本地socket发送数据");

    pthread_join(collect_tid,NULL);
    pthread_join(client_tid,NULL);
    pthread_mutex_destroy(&g_data_mutex);
    LOG_COLLECT_INFO("程序正常退出");
    return 0;
}


//tail -f /var/log/messages | grep iot_collect