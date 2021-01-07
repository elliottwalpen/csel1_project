#include <stdio.h>
#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <stdint.h>
#include <errno.h>
#include <syslog.h>$

#include "fan.h"

#define FAN_PATH             "/sys/devices/platform/fan-driver/"


static int current_mode = AUTO;
static int current_freq = TWO;

static const char* sys_name (const char* path, const char* attr)
{
    static char name[512] = {[0] = 0,};

    strncpy (name, path,      sizeof(name)-1);
    strncat (name, attr,      sizeof(name)-strlen(name)-1);

    return name;
}

int init_fan(void)
{
    /*int fd = open(MODULE_PATH, O_RDONLY);
    if(fd < 0) {
        perror("Unable to load module\n");
        return EXIT_FAILURE;
    }
    else {
        //if(finit_module(fd, "", 0) != 0) {
         //   perror("Unable to load module\n");
         //   return EXIT_FAILURE;
        //}
        close(fd);
    }*/
    /* TODO set default configuration */
    driver_set_mode(current_mode);
    return EXIT_SUCCESS;
}

float driver_get_temperature(void)
{
    char buf[100];
    float temp = 0.0f;
    int fd = open(SYS_ATTR_PATH CPU_TEMP, O_RDONLY);

    if(fd > -1)
        read(fd, buf, sizeof(buf));
    else
        syslog(LOG_INFO, "Failed to open temp attribute %s\n", strerror(errno));

    close(fd);

    temp = strtof(buf, NULL) / 1000;
    return temp;
}

int driver_get_freq(void)
{
    char buf[100];
    int freq = 0.0f;
    int fd = open(SYS_ATTR_PATH FAN_FREQ, O_RDONLY);

    if(fd > -1)
        read(fd, buf, sizeof(buf));
    else
        syslog(LOG_INFO, "Failed to open freq attribute %s\n", strerror(errno));

    close(fd);

    freq = atoi(buf);
    return freq; 
}

int driver_get_mode(void)
{
    char buf[100];
    int mode = 0.0f;
    int fd = open(SYS_ATTR_PATH FAN_MODE, O_RDONLY);

    if(fd > -1)
        read(fd, buf, sizeof(buf));
    else
        syslog(LOG_INFO, "Failed to open mode attribute %s\n", strerror(errno));

    close(fd);

    mode = atoi(buf);
    return mode; 
}

void driver_set_mode(int mode)
{
    char buf[100];
    int fd = open(SYS_ATTR_PATH FAN_MODE, O_WRONLY);

    if(fd > -1) {
        sprintf(buf, "%d\n", mode);
        write(fd, buf, sizeof(buf));
        current_mode = mode;
    }
    else {
        syslog(LOG_INFO, "Failed to open mode attribute %s\n", strerror(errno));
    }

    close(fd);
}

void driver_set_freq(int freq)
{
    char buf[100];
    if(current_mode == AUTO)
        return -1;

    int fd = open(SYS_ATTR_PATH FAN_FREQ, O_WRONLY);

    if(fd > -1) {
        sprintf(buf, "%d\n", freq);
        write(fd, buf, sizeof(buf));
        current_freq = freq;
    }
    else {
        syslog(LOG_INFO, "Failed to open freq attribute %s\n", strerror(errno));
    }

    close(fd);
}

void clean_fan(void)
{
    /* Drv can be remove here */
}