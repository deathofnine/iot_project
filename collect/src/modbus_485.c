#include "modbus.h"
modbus_t *ctx = NULL;
int mb485_init(const char *dev, int baud, int slave_id)
{
    ctx = modbus_new_rtu(dev, baud, 'N', 8, 1);
    if (!ctx)
    {
        syslog(LOG_ERR, "modbus new rtu fail: %s", modbus_strerror(errno));
        return -1;
    }
    modbus_set_slave(ctx, slave_id);
    if (modbus_connect(ctx) == -1)
    {
        syslog(LOG_ERR, "modbus connect fail: %s", modbus_strerror(errno));
        modbus_free(ctx);
        ctx = NULL;
        return -1;
    }
    // 你485 DE/RE控制引脚初始化（复用485 GPIO导出代码）
    mb_gpio_de_re_init();
    syslog(LOG_INFO, "modbus 485 init ok dev:%s", dev);
    return 0;
}

// 读取保持寄存器，存入g_local_mb
int mb485_read_data(void)
{
    uint16_t reg_buf[4] = {0};
    int ret = modbus_read_registers(ctx, 0, 4, reg_buf);
    if(ret < 0)
    {
        syslog(LOG_WARNING, "modbus read reg fail:%s", modbus_strerror(errno));
        return -1;
    }

    // 上锁更新本地modbus缓存
    pthread_mutex_lock(&local_mb_mtx);
    g_local_mb.reg0 = reg_buf[0];
    g_local_mb.reg1 = reg_buf[1];
    g_local_mb.reg2 = reg_buf[2];
    g_local_mb.reg3 = reg_buf[3];
    g_local_mb.voltage = reg_buf[0] / 10.0f;
    g_local_mb.current = reg_buf[1] / 100.0f;
    pthread_mutex_unlock(&local_mb_mtx);
    return 0;
}