#include "gpio.h"
#include "st7735s.h"
#include "spi.h"
#include <unistd.h>
#include <linux/spi/spidev.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// //
// // Internal helpers
// //

static void write_cmd(st7735s_t *lcd, uint8_t cmd)
{
    gpio_set(lcd->pin_dc, 0);
    spi_write_chunked(&lcd->spi, &cmd, 1);
}

static void write_data(st7735s_t *lcd, uint8_t data)
{
    gpio_set(lcd->pin_dc, 1);
    spi_write_chunked(&lcd->spi, &data, 1);
}

static void write_data_buf(st7735s_t *lcd, uint8_t *buf, size_t len)
{
    gpio_set(lcd->pin_dc, 1);
    spi_write_chunked(&lcd->spi, buf, len);
}

//
// Initialization sequence (ST7735S typical)
//

static const uint8_t init_seq[] = {
    // SWRESET
    1, 0x01,
    0, 150,   // delay 150ms

    // SLPOUT
    1, 0x11,
    0, 150,

    // COLMOD = 16-bit
    2, 0x3A, 0x05,

    // MADCTL = row/column order
    2, 0x36, 0xC8,  // RGB order + orientation

    // DISPON
    1, 0x29,
    0, 100,

    0xFF           // end marker
};

static void run_init_sequence(st7735s_t *lcd)
{
    const uint8_t *p = init_seq;

    while (1) {
        uint8_t count = *p++;

        if (count == 0xFF)
            break;

        if (count > 0) {
            uint8_t cmd = *p++;
            write_cmd(lcd, cmd);

            for (int i = 1; i < count; i++)
                write_data(lcd, *p++);
        } else {
            // delay
            uint8_t ms = *p++;
            usleep(ms * 1000);
        }
    }
}

//
// Public API
//

int st7735s_init(st7735s_t *lcd,
                 const char *spi_dev,
                 int gpio_dc,
                 int gpio_reset)
{
    memset(lcd, 0, sizeof(*lcd));

    lcd->pin_dc = gpio_dc;
    lcd->pin_reset = gpio_reset;

    gpio_request_output(gpio_dc);
    gpio_request_output(gpio_reset);

    // Hardware reset pulse
    gpio_set(gpio_reset, 0);
    usleep(20000);
    gpio_set(gpio_reset, 1);
    usleep(20000);

    // Init SPI
    if (spi_init(&lcd->spi, spi_dev,
                 SPI_MODE_0,
                 16000000,
                 8) < 0) {
        return -1;
    }

    // Run LCD init sequence
    run_init_sequence(lcd);

    // Clear screen black
    st7735s_fill_screen(lcd, 0x0000);

    return 0;
}

void st7735s_set_addr_window(st7735s_t *lcd,
                             uint8_t x0, uint8_t y0,
                             uint8_t x1, uint8_t y1)
{
    write_cmd(lcd, 0x2A); // CASET
    write_data(lcd, 0);
    write_data(lcd, x0 + ST7735S_X_OFFSET);
    write_data(lcd, 0);
    write_data(lcd, x1 + ST7735S_X_OFFSET);

    write_cmd(lcd, 0x2B); // RASET
    write_data(lcd, 0);
    write_data(lcd, y0 + ST7735S_Y_OFFSET);
    write_data(lcd, 0);
    write_data(lcd, y1 + ST7735S_Y_OFFSET);

    write_cmd(lcd, 0x2C); // RAMWR
}

void st7735s_draw_pixel(st7735s_t *lcd,
                        uint8_t x, uint8_t y,
                        uint16_t color)
{
    st7735s_set_addr_window(lcd, x, y, x, y);
    uint8_t buf[2] = { color >> 8, color & 0xFF };
    write_data_buf(lcd, buf, 2);
}

void st7735s_fill_rect(st7735s_t *lcd,
                       uint8_t x, uint8_t y,
                       uint8_t w, uint8_t h,
                       uint16_t color)
{
    st7735s_set_addr_window(lcd, x, y, x + w - 1, y + h - 1);

    size_t pixels = w * h;
    size_t bytes = pixels * 2;

    uint8_t *buf = malloc(bytes);
    for (int i = 0; i < pixels; i++) {
        buf[2*i]   = color >> 8;
        buf[2*i+1] = color & 0xFF;
    }

    write_data_buf(lcd, buf, bytes);
    free(buf);
}

void st7735s_fill_screen(st7735s_t *lcd, uint16_t color)
{
    st7735s_fill_rect(lcd, 0, 0,
                      ST7735S_WIDTH,
                      ST7735S_HEIGHT,
                      color);
}

void st7735s_draw_hline(st7735s_t *lcd,
                        int x, int y,
                        int w,
                        uint16_t color)
{
    if (w < 0) {
        x += w;
        w = -w;
    }

    st7735s_set_addr_window(lcd, x, y, x + w - 1, y);
    uint8_t buf[w * 2];

    for (int i = 0; i < w; i++) {
        buf[2*i]   = color >> 8;
        buf[2*i+1] = color & 0xFF;
    }

    write_data_buf(lcd, buf, w * 2);
}

void st7735s_draw_vline(st7735s_t *lcd,
                        int x, int y,
                        int h,
                        uint16_t color)
{
    if (h < 0) {
        y += h;
        h = -h;
    }

    st7735s_set_addr_window(lcd, x, y, x, y + h - 1);
    uint8_t buf[h * 2];

    for (int i = 0; i < h; i++) {
        buf[2*i]   = color >> 8;
        buf[2*i+1] = color & 0xFF;
    }

    write_data_buf(lcd, buf, h * 2);
}

void st7735s_push_framebuffer(st7735s_t *lcd,
                              uint16_t *fb,
                              int w, int h)
{
    // Set window to full screen
    st7735s_set_addr_window(lcd, 0, 0, w - 1, h - 1);

    // Data mode
    gpio_set(lcd->pin_dc, 1);

    // Send entire framebuffer in one big SPI write
    spi_write_chunked(&lcd->spi,
                      (uint8_t*)fb,
                      w * h * sizeof(uint16_t));
}

void st7735s_draw_line(st7735s_t *lcd,
                       int x0, int y0,
                       int x1, int y1,
                       uint16_t color)
{
    int dx = abs(x1 - x0);
    int sx = x0 < x1 ? 1 : -1;
    int dy = -abs(y1 - y0);
    int sy = y0 < y1 ? 1 : -1;

    int err = dx + dy;
    int e2;

    while (1) {
        st7735s_draw_pixel(lcd, x0, y0, color);

        if (x0 == x1 && y0 == y1)
            break;

        e2 = 2 * err;

        if (e2 >= dy) {
            err += dy;
            x0 += sx;
        }

        if (e2 <= dx) {
            err += dx;
            y0 += sy;
        }
    }
}

void st7735s_draw_circle(st7735s_t *lcd, uint8_t radius, 
                         uint16_t X0, uint16_t Y0, 
                         uint16_t color){
	int x = 0;
	int y = radius;
	int d = 3-2*radius;

	while(x<=y){
		st7735s_draw_pixel(lcd, X0 + x, Y0 + y, color);
		st7735s_draw_pixel(lcd, X0 - x, Y0 + y, color);
		st7735s_draw_pixel(lcd, X0 + x, Y0 - y, color);
		st7735s_draw_pixel(lcd, X0 - x, Y0 - y, color);

		st7735s_draw_pixel(lcd, X0 + y, Y0 + x, color);
		st7735s_draw_pixel(lcd, X0 - y, Y0 + x, color);
		st7735s_draw_pixel(lcd, X0 + y, Y0 - x, color);
		st7735s_draw_pixel(lcd, X0 - y, Y0 - x, color);

		if(d < 0){
			d+=4*x+6;
		}else{
			d+=4*(x-y)+10;
			y--;
		}

		x++;
	}
}
