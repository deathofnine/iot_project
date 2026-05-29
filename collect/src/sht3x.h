#ifndef __SHT3X_H__
#define __SHT3X_H__

#include "common.h"   

int sht3x_init(const char *i2c_dev);
int sht3x_read(int fd, float *temp, float *hum);


#endif