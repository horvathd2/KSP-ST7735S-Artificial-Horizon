#ifndef __ST7735S_H
#define __ST7735S_H

#include <stdint.h>
#include "spi.h"

// Display resolution (common ST7735S)
#define ST7735S_WIDTH   128
#define ST7735S_HEIGHT  160

#define ST7735S_X_OFFSET 2
#define ST7735S_Y_OFFSET 1

typedef struct {
    spi_device_t spi;
    int pin_dc;
    int pin_reset;
} st7735s_t;

int st7735s_init(st7735s_t *lcd,
                 const char *spi_dev,
                 int gpio_dc,
                 int gpio_reset);

void st7735s_set_addr_window(st7735s_t *lcd,
                             uint8_t x0, uint8_t y0,
                             uint8_t x1, uint8_t y1);

void st7735s_fill_rect(st7735s_t *lcd,
                       uint8_t x, uint8_t y,
                       uint8_t w, uint8_t h,
                       uint16_t color);

void st7735s_fill_screen(st7735s_t *lcd, uint16_t color);

void st7735s_draw_pixel(st7735s_t *lcd,
                        uint8_t x, uint8_t y,
                        uint16_t color);

#endif
