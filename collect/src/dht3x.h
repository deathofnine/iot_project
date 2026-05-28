#ifndef __DHT3X_H__
#define __DHT3X_H__

#include "common.h"   

int dht3x_init(const char *i2c_dev);
int dht3x_read(int fd, float *temp, float *hum);

#endif