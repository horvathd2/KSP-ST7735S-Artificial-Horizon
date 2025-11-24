#include "gpio.h"
#include <gpiod.h>
#include <stdio.h>
#include <stdlib.h>

static struct gpiod_chip *chip = NULL;
static struct gpiod_line *lines[64] = {0};

int gpio_init_chip(const char *chip_name)
{
    chip = gpiod_chip_open(chip_name);
    if (!chip) {
        perror("gpiod_chip_open");
        return -1;
    }
    return 0;
}

int gpio_request_output(int pin)
{
    if (!chip) {
        if (gpio_init_chip("/dev/gpiochip0") < 0)
            return -1;
    }

    lines[pin] = gpiod_chip_get_line(chip, pin);
    if (!lines[pin]) {
        perror("gpiod_chip_get_line");
        return -1;
    }

    if (gpiod_line_request_output(lines[pin], "ksp-horizon", 0) < 0) {
        perror("gpiod_line_request_output");
        return -1;
    }

    return 0;
}

void gpio_set(int pin, int value)
{
    if (!lines[pin]) return;

    if (gpiod_line_set_value(lines[pin], value) < 0)
        perror("gpiod_line_set_value");
}
