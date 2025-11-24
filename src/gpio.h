#ifndef __GPIO_H
#define __GPIO_H

int gpio_init_chip(const char *chip_name);
int gpio_request_output(int pin);
void gpio_set(int pin, int value);

#endif
