#pragma once
#ifndef FAN_H
#define FAN_H

#define FAN_FREQ    "/freq"
#define FAN_MODE    "/mode"
#define CPU_TEMP    "/temp"
#define SYS_ATTR_PATH "/sys/devices/platform/fan_driver"
#define MODULE_PATH "/lib/modules/5.8.6/extra/fan_driver.ko"
#define MODULE_NAME "fan_driver"

typedef enum {MANUAL = 0, AUTO = 1} mode;
typedef enum {TWO = 2, FIVE = 5, TEN = 10, TWENTY = 20} freq;

int init_fan(void);

int deinit_fan(void);

float driver_get_temperature(void);

int driver_get_freq(void);

int driver_get_mode(void);

void driver_set_mode(int mode);

int driver_set_freq(int freq);


#endif