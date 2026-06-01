#include "common.h"
#include "MQTTClient.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <cJSON.h>

// ---------- 华为云IoT 新设备参数 ----------
#define MQTT_HOST     "b1d7c0faf7.st1.iotda-device.cn-north-4.myhuaweicloud.com"
#define MQTT_PORT     8883
#define CLIENT_ID     "6a00bc177f2e6c302f6f8843_imx6ull_sensor_001_0_0_2026060114"
#define USER_NAME     "6a00bc177f2e6c302f6f8843_imx6ull_sensor_001"
#define PASSWORD      "60b30cc1d11132dce2e8194019a952253346893dbc1c4afebe8e8254998a317d"
// 华为云IoT 物模型上报主题（固定格式）
  #define TOPIC_POST "$oc/devices/6a00bc177f2e6c302f6f8843_imx6ull_sensor_001/sys/properties/report"

"
#define MQTT_QOS           0
#define MQTT_KEEPALIVE     60
// ==========================================================

static MQTTClient client;
static int mqtt_connected = 0;

static int mqtt_connect_broker(void)
{
    int rc;
    MQTTClient_connectOptions conn_opts = MQTTClient_connectOptions_initializer;
    MQTTClient_SSLOptions ssl_opts = MQTTClient_SSLOptions_initializer;  // 单独定义，修复编译错误

    char broker_url[128];
    snprintf(broker_url, sizeof(broker_url), "ssl://%s:%d", MQTT_HOST, MQTT_PORT);
    rc = MQTTClient_create(&client, broker_url, CLIENT_ID,
                          MQTTCLIENT_PERSISTENCE_NONE, NULL);
    if (rc != MQTTCLIENT_SUCCESS) {
        printf("[MQTT] create failed %d\n", rc);
        return -1;
    }

    conn_opts.keepAliveInterval = MQTT_KEEPALIVE;
    conn_opts.cleansession      = 1;
    conn_opts.username          = USER_NAME;
    conn_opts.password          = PASSWORD;

    // SSL 正确配置
    ssl_opts.enableServerCertAuth = 0;
    conn_opts.ssl = &ssl_opts;  // 传递指针（关键修复）

    rc = MQTTClient_connect(client, &conn_opts);
    if (rc != MQTTCLIENT_SUCCESS) {
        printf("[MQTT] connect failed %d\n", rc);
        MQTTClient_destroy(&client);
        mqtt_connected = 0;
        return -1;
    }

    printf("[MQTT] 华为云连接成功 ✅\n");
    mqtt_connected = 1;
    return 0;
}

static void mqtt_upload_data(void)
{
    if (!mqtt_connected) return;

    pthread_mutex_lock(&g_data_mutex);
    sensor_data_t data = g_sensor;
    pthread_mutex_unlock(&g_data_mutex);

    char payload[256];
    snprintf(payload, sizeof(payload),
        "{"
            "\"services\": [{"
                "\"service_id\": \"DeviceData\","
                "\"properties\": {"
                    "\"temp\": %.1f,"
                    "\"hum\": %.1f,"
                    "\"ax\": %.2f,"
                    "\"ay\": %.2f,"
                    "\"az\": %.2f,"
                    "\"gx\": %.2f,"
                    "\"gy\": %.2f,"
                    "\"gz\": %.2f"
                "}"
            "}]"
        "}",
        data.temp, data.hum,
        data.ax, data.ay, data.az,
        data.gx, data.gy, data.gz);

    MQTTClient_message pubmsg = MQTTClient_message_initializer;
    pubmsg.payload = payload;
    pubmsg.payloadlen = strlen(payload);
    pubmsg.qos = MQTT_QOS;
    pubmsg.retained = 0;

    MQTTClient_deliveryToken token;
    int rc = MQTTClient_publishMessage(client, TOPIC_POST, &pubmsg, &token);
    if (rc == MQTTCLIENT_SUCCESS) {
        MQTTClient_waitForCompletion(client, token, 2000);
        printf("[MQTT] 上报成功: %s\n", payload);
    } else {
        printf("[MQTT] 上报失败 %d\n", rc);
        mqtt_connected = 0;
    }
}

void *mqtt_upload_thread(void *arg)
{
    (void)arg;
    while (1) {
        if (!mqtt_connected) {
            mqtt_connect_broker();
            sleep(2);
            continue;
        }
        mqtt_upload_data();
        sleep(1);
    }
    return NULL;
}