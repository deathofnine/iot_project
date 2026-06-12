#ifndef _LOG_H_
#define _LOG_H_

// 日志级别
typedef enum {
    LOG_LEVEL_DEBUG = 0,
    LOG_LEVEL_INFO  = 1,
    LOG_LEVEL_WARN  = 2,
    LOG_LEVEL_ERROR = 3
} log_level_t;

// 日志类型（不同模块写不同文件）
typedef enum {
    LOG_TYPE_COLLECT = 0,   // 采集日志 /var/log/iot/collect.log
    LOG_TYPE_SOCKET  = 1,   // Socket日志 /var/log/iot/socket.log
    LOG_TYPE_ERROR   = 2    // 错误日志 /var/log/iot/error.log
} log_type_t;

// 初始化日志系统
void log_init(void);

// 写日志（一般用宏，不直接调用）
void log_write(log_type_t type, log_level_t level, const char *file, int line, const char *fmt, ...);

// 关闭日志系统
void log_close(void);

// 便捷宏
#define LOG_COLLECT_DEBUG(fmt, ...)  log_write(LOG_TYPE_COLLECT, LOG_LEVEL_DEBUG, __FILE__, __LINE__, fmt, ##__VA_ARGS__)
#define LOG_COLLECT_INFO(fmt, ...)   log_write(LOG_TYPE_COLLECT, LOG_LEVEL_INFO,  __FILE__, __LINE__, fmt, ##__VA_ARGS__)
#define LOG_COLLECT_WARN(fmt, ...)   log_write(LOG_TYPE_COLLECT, LOG_LEVEL_WARN,  __FILE__, __LINE__, fmt, ##__VA_ARGS__)
#define LOG_COLLECT_ERROR(fmt, ...)  log_write(LOG_TYPE_COLLECT, LOG_LEVEL_ERROR, __FILE__, __LINE__, fmt, ##__VA_ARGS__)

#define LOG_SOCKET_DEBUG(fmt, ...)   log_write(LOG_TYPE_SOCKET,  LOG_LEVEL_DEBUG, __FILE__, __LINE__, fmt, ##__VA_ARGS__)
#define LOG_SOCKET_INFO(fmt, ...)    log_write(LOG_TYPE_SOCKET,  LOG_LEVEL_INFO,  __FILE__, __LINE__, fmt, ##__VA_ARGS__)
#define LOG_SOCKET_WARN(fmt, ...)    log_write(LOG_TYPE_SOCKET,  LOG_LEVEL_WARN,  __FILE__, __LINE__, fmt, ##__VA_ARGS__)
#define LOG_SOCKET_ERROR(fmt, ...)   log_write(LOG_TYPE_SOCKET,  LOG_LEVEL_ERROR, __FILE__, __LINE__, fmt, ##__VA_ARGS__)

#define LOG_ERROR(fmt, ...)          log_write(LOG_TYPE_ERROR,   LOG_LEVEL_ERROR, __FILE__, __LINE__, fmt, ##__VA_ARGS__)
#define LOG_ERROR_ERRNO(fmt, ...)    LOG_ERROR(fmt " : %s", ##__VA_ARGS__, strerror(errno))

#endif