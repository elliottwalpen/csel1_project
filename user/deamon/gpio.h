#pragma once
#ifndef GPIO_H
#define GPIO_H

#define GPIO_GPIO "/sys/class/gpio/gpio"
#define GPIO_EXPORT	"/sys/class/gpio/export"
#define GPIO_UNEXPORT "/sys/class/gpio/unexport"
#define NON_EDGE "nonedge"

int led_fd;

#define K1_NB "0"
#define K2_NB "2"
#define K3_NB "3"

#define LED "362"
#define LED_ON "1"
#define LED_OFF "0"

int configure_gpio(const char* nr, const char* dir, const char* edge);

void free_gpio(const char* nr);
#endif