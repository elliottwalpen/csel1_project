// fan_driver.c
/*
 This module use 3 attributes available under /sys/devices/platform/fan_driver/ :
    - temp : cpu temperature as int, read only
    - freq : fan frequency in hz constrained to 2, 5, 10 or 20, read and write (in manual mode)
    - mode : manual (0) or automatic mode (1), read and write

access attributes with "cat" and "echo" with the corresponding file

remember to insert the module with "insmod fan_driver.ko" or "modprobe fan_driver"
*/
#include <linux/module.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/device.h>
#include <linux/platform_device.h>
#include <linux/slab.h>

#include <linux/gpio.h>
#include <linux/thermal.h>
#include <linux/timer.h>

static int period_ms = 250; // 2hz default
static int is_auto = 1; // auto mode default
static int freq_index = 0;
static int temperature;
static int DEC_FREQ[] = {2, 5, 10, 20};
static int DEC_PERIOD[] = {250, 100, 50 ,25};
#define LED_STATUS_PIN 10
#define TEMP_LOW 35000
#define TEMP_INTER 40000
#define TEMP_HIGH 45000

/*-------- Peripherals access --------*/
void toggle_led(int value)
{
    if(gpio_request(LED_STATUS_PIN, "StatusLed") == 0)
    {
        gpio_direction_output(LED_STATUS_PIN, value);
        gpio_free(LED_STATUS_PIN);
    }
}

int get_cpu_temperature(void)
{
    int te = 0;  
    int ret = thermal_zone_get_temp(thermal_zone_get_zone_by_name("cpu_thermal"), &temperature);
    //pr_info("Return code : %d && Temperature %d \t", ret, temperature);
    return 33000;
}

struct timer_list mytimer;
static void timer_callback(struct timer_list *timer)
{
    static int led_value = 0;
    if(is_auto == 1)
    {
        int t = get_cpu_temperature();
        //pr_info("Temperature %d \t", t);
        if(t < TEMP_LOW)
            period_ms = DEC_PERIOD[0];
        else if(t < TEMP_INTER)
            period_ms = DEC_PERIOD[1];
        else if(t < TEMP_HIGH)
            period_ms = DEC_PERIOD[2];
        else
            period_ms = DEC_PERIOD[3];    
    }
    else
    {
        period_ms = DEC_PERIOD[freq_index];
    }
    mod_timer(&mytimer, jiffies + msecs_to_jiffies(period_ms));
    toggle_led(led_value);
    led_value = led_value ? 0:1;
}

/*-------- Attribute mgmt --------*/
#define MODE_MANUAL "0\n"
#define MODE_AUTO "1\n"

const char* FREQUENCIES[] = {"2\n", "5\n", "10\n", "20\n"};

static char temp_buf[1000];
static char freq_buf[1000];
static char mode_buf[1000];

// read access
ssize_t show_temp(struct device* dev, struct device_attribute* attr, char* buf)
{
    sprintf(temp_buf, "%d\n", get_cpu_temperature());
    strcpy(buf, temp_buf);
    return strlen(buf);
}

ssize_t show_freq(struct device* dev, struct device_attribute* attr, char* buf)
{
    strcpy(buf, freq_buf);
    return strlen(buf);
}

ssize_t show_mode(struct device* dev, struct device_attribute* attr, char* buf)
{
    strcpy(buf, mode_buf);
    return strlen(buf);
}

// write access
ssize_t store_freq(struct device* dev, struct device_attribute* attr, const char* buf, size_t count)
{
    int len = sizeof(freq_buf) - 1;
    if(is_auto == 1)
    {
        pr_info("Write not allowed in automatic mode\n");
        return len;
    }
    int i = 0;
    for(i=0; i<(sizeof(FREQUENCIES)/sizeof(FREQUENCIES[0])); i++)
    {
        pr_info("Index is %d : ", i);
        if(strcmp(buf, FREQUENCIES[i]) == 0)
        {
            pr_info("Freq stored : %s", buf);
            freq_index = i;
            int len = sizeof(freq_buf) - 1;
            if(len > count) len = count;
            strncpy(freq_buf, buf, len);
            freq_buf[len] = 0;
            return len;
        }
    }
    pr_info("Invalid value\n");
    return len;
}

ssize_t store_mode(struct device* dev, struct device_attribute* attr, const char* buf, size_t count)
{
    int len = len = sizeof(mode_buf) - 1;
    if(strcmp(buf, MODE_AUTO) == 0)
        is_auto = 1;
    else if(strcmp(buf, MODE_MANUAL) == 0)
        is_auto = 0;
    else {
        pr_info("Invalid value, type 0 for manual or 1 for automatic\n");
        return len;
    }
    pr_info("Mode stored : %s", buf);
    len = sizeof(mode_buf) - 1;
    if(len > count) len = count;
    strncpy(mode_buf, buf, len);
    mode_buf[len] = 0;
    return len;
}

DEVICE_ATTR(temp, 0444, show_temp, NULL); // dev_attr_temp read-only
DEVICE_ATTR(freq, 0664, show_freq, store_freq); // dev_attr_freq
DEVICE_ATTR(mode, 0664, show_mode, store_mode);  // dev_attr_mode

static void sysfs_dev_release(struct device* dev) {}

static struct platform_device fan_driver = {
    .name = "fan_driver",
    .id = -1,
    .dev.release = sysfs_dev_release,
};

static int __init fan_driver_init(void)
{
    pr_info ("Linux module loaded \n");
    int status = 0;
    if (status == 0)
        status = platform_device_register(&fan_driver);
    if(status == 0)
        /* init all attributes*/
        status = device_create_file(&fan_driver.dev, &dev_attr_temp);
    if(status == 0)
        status = device_create_file(&fan_driver.dev, &dev_attr_freq);
    if(status == 0)
        status = device_create_file(&fan_driver.dev, &dev_attr_mode);
    if(status == 0)
        strcpy(mode_buf, MODE_AUTO); // start with automatic mode
        is_auto = 1;
        freq_index = 0;
        period_ms = 250; // 1/2Hz % 2
        strcpy(freq_buf, FREQUENCIES[freq_index]); // start with 2Hz

        /* init timer */
        timer_setup(&mytimer, timer_callback, 0);
        mod_timer(&mytimer, jiffies + msecs_to_jiffies(period_ms));

    return status;
}

static void __exit fan_driver_exit(void)
{
    /* stop timer */
    del_timer(&mytimer);
    pr_info("Timer stopped\n");
    toggle_led(0);
    pr_info("Led off\n");
    /* remove attributes */
    device_remove_file(&fan_driver.dev, &dev_attr_temp);
    pr_info("Attribute temp removed \n");
    device_remove_file(&fan_driver.dev, &dev_attr_freq);
    pr_info("Attribute freq  removed \n");
    device_remove_file(&fan_driver.dev, &dev_attr_mode);
    pr_info("Attribute mode removed \n");
    platform_device_unregister(&fan_driver);
    pr_info ("Linux module unloaded\n");
}

module_init (fan_driver_init);
module_exit (fan_driver_exit);

MODULE_AUTHOR ("Elliott Walpen <elliott.walpen@hes-so.ch>");
MODULE_DESCRIPTION ("Module sysfs for fan driver");
MODULE_LICENSE ("GPL");

