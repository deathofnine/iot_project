#include "log.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <stdarg.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <pthread.h>
#include <errno.h>

#define LOG_DIR         "/var/log/iot"
#define LOG_FILE_COLLECT LOG_DIR "/collect.log"
#define LOG_FILE_SOCKET  LOG_DIR "/socket.log"
#define LOG_FILE_ERROR   LOG_DIR "/error.log"

//单个日志文件最大大小
#define MAX_LOG_SIZE    (1 * 1024 * 1024)

// 保留的备份文件数量
#define MAX_BACKUP_COUNT 3

static pthread_mutex_t log_mutex = PTHREAD_MUTEX_INITIALIZER;

// 获取当前时间字符串
static void get_time_str(char *buf, size_t size)
{
    time_t now = time(NULL);
    struct tm *tm = localtime(&now);
    strftime(buf, size, "%Y-%m-%d %H:%M:%S", tm);
}

// 获取日志级别字符串
static const char *get_level_str(log_level_t level)
{
    switch (level) {
        case LOG_LEVEL_DEBUG: return "DEBUG";
        case LOG_LEVEL_INFO:  return "INFO";
        case LOG_LEVEL_WARN:  return "WARN";
        case LOG_LEVEL_ERROR: return "ERROR";
        default:              return "UNKN";
    }
}

// 根据类型获取日志文件路径
static const char *get_log_path(log_type_t type)
{
    switch (type) {
        case LOG_TYPE_COLLECT: return LOG_FILE_COLLECT;
        case LOG_TYPE_SOCKET:  return LOG_FILE_SOCKET;
        case LOG_TYPE_ERROR:   return LOG_FILE_ERROR;
        default:               return LOG_FILE_ERROR;
    }
}

// 轮转日志文件
static void rotate_log_file(const char *logfile)
{
    struct stat st;
    if (stat(logfile, &st) != 0 || st.st_size < MAX_LOG_SIZE) {
        return;  // 文件不存在或大小未超限
    }
    
    char old_path[256], new_path[256];
    
    // 删除最旧的备份
    snprintf(old_path, sizeof(old_path), "%s.%d", logfile, MAX_BACKUP_COUNT);
    unlink(old_path);
    
    // 轮转：.2 → .3, .1 → .2, .log → .1
    for (int i = MAX_BACKUP_COUNT - 1; i >= 1; i--) {
        snprintf(old_path, sizeof(old_path), "%s.%d", logfile, i);
        snprintf(new_path, sizeof(new_path), "%s.%d", logfile, i + 1);
        rename(old_path, new_path);
    }
    
    // 当前日志文件改为 .1
    snprintf(new_path, sizeof(new_path), "%s.1", logfile);
    rename(logfile, new_path);
}

// 初始化日志系统
void log_init(void)
{
    // 创建日志目录
    mkdir(LOG_DIR, 0755);
    
    // 检查并轮转现有日志文件
    rotate_log_file(LOG_FILE_COLLECT);
    rotate_log_file(LOG_FILE_SOCKET);
    rotate_log_file(LOG_FILE_ERROR);
}

// 核心写日志函数
void log_write(log_type_t type, log_level_t level, const char *file, int line, const char *fmt, ...)
{
    pthread_mutex_lock(&log_mutex);
    
    const char *log_path = get_log_path(type);
    if (!log_path) {
        pthread_mutex_unlock(&log_mutex);
        return;
    }
    
    // 检查是否需要轮转
    rotate_log_file(log_path);
    
    FILE *fp = fopen(log_path, "a");
    if (!fp) {
        pthread_mutex_unlock(&log_mutex);
        return;
    }
    
    // 时间戳
    char time_str[32];
    get_time_str(time_str, sizeof(time_str));
    
    // 级别
    const char *level_str = get_level_str(level);
    
    // 只取文件名（去掉路径）
    const char *filename = strrchr(file, '/');
    if (filename) filename++; else filename = file;
    
    // 打印时间、级别、位置
    fprintf(fp, "[%s] [%s] [%s:%d] ", time_str, level_str, filename, line);
    
    // 打印用户消息
    va_list args;
    va_start(args, fmt);
    vfprintf(fp, fmt, args);
    va_end(args);
    
    fprintf(fp, "\n");
    fflush(fp);
    fclose(fp);
    
    pthread_mutex_unlock(&log_mutex);
}

void log_close(void)
{
    // 无需额外操作
}