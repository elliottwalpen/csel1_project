#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>

#include "gpio.h"
#include "fan.h"

#include <syslog.h>
#define ARRAY_SIZE(x) (sizeof(x)/sizeof(x[0]))

static const char* sys_name(const char* path, const char* nr, const char* attr)
{
    static char name[512] = {[0] = 0,};

    // compute path (concat path with attribute)
    strncpy(name, path, sizeof(name)-1);
    strncat(name, nr, sizeof(name)-strlen(name)-1);
    strncat(name, attr, sizeof(name)-strlen(name)-1);
    
    return name;
}

int configure_gpio(const char* nr, const char* dir, const char* edge) 
{

	// export pin to sysfs (reinitialization)
	int fd = open(GPIO_UNEXPORT, O_WRONLY);
    syslog(LOG_INFO, "Fd unexport %d\n", fd);
	write(fd, nr, strlen(nr));
	close(fd);
    
    // export pin to sysfs
    fd = open(GPIO_EXPORT, O_WRONLY);
    syslog(LOG_INFO, "Fd export %d\n", fd);
    write(fd, nr, strlen(nr));
    close(fd);
    
    // configure pin direction 
    fd = open(sys_name(GPIO_GPIO, nr, "/direction"), O_WRONLY);
    syslog(LOG_INFO, "Fd configure pin %d\n", fd);
    write(fd, dir, strlen(dir));
    close(fd);
    
    // configure edge signal
    if(strcmp(edge, NON_EDGE))
    {
        fd = open(sys_name(GPIO_GPIO, nr, "/edge"), O_RDWR);
        syslog(LOG_INFO, "Fd configure edge %d\n", fd);
        write(fd, edge, strlen(edge));
        close(fd);
    }
        
    // open value attribute
    if(!strcmp(edge, NON_EDGE))
    {
        fd = open(sys_name(GPIO_GPIO, nr,"/value"), O_RDWR);
        syslog(LOG_INFO, "Fd open  value %d\n", fd);
    } else fd = open(sys_name(GPIO_GPIO, nr,"/value"), O_RDONLY);

    char buf[10];
    pread(fd, buf, ARRAY_SIZE(buf), 0);
    
    return fd;
}

void free_gpio (const char* nr) 
{
    int fd = open (GPIO_UNEXPORT, O_WRONLY);
    write (fd, nr, strlen(nr));
    close (fd);   
}