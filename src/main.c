#include "st7735s.h"
#include "gpio.h"
#include <stdio.h>
#include <unistd.h>
#include <malloc.h>
#include <pthread.h>
#include <stdint.h>
#include <fcntl.h>
#include <termios.h>
#include <string.h>

#define START_BYTE 0xAA

static uint8_t buffer[14];
static int state = 0;
static int idx = 0;

void process_packet(uint8_t* buf) {
    int32_t pitch, roll, yaw;

    memcpy(&pitch, &buf[1], 4);
    memcpy(&roll,  &buf[5], 4);
    memcpy(&yaw,   &buf[9], 4);
    uint8_t checksum = buf[13];

    uint8_t calc = 0;
    for (int i = 1; i < 13; i++)
        calc += buf[i];

    if (calc != checksum) {
        printf("Checksum error: got %02X, expected %02X\n", checksum, calc);
        return;
    }

    printf("Pitch: %d, Roll: %d, Yaw: %d\n", pitch, roll, yaw);
}

void *lcd_main(void *arguments){
    st7735s_t horizon_lcd;

    if (st7735s_init(&horizon_lcd,
                    "/dev/spidev0.0",
                    24,      // DC GPIO
                    25)) {    // RESET GPIO
        printf("LCD init failed...\n");
        return NULL;
    }

    while(1){
        st7735s_fill_screen(&horizon_lcd, 0xF800); // Red
        sleep(1);

        st7735s_fill_screen(&horizon_lcd, 0x07E0); // Green
        sleep(1);

        st7735s_fill_screen(&horizon_lcd, 0x001F); // Blue
        sleep(1);

        st7735s_draw_pixel(&horizon_lcd, 10, 10, 0xFFFF);
    }

    return NULL;
}

void *uart_main(void *arguments){
    int fd = open("/dev/ttyUSB0", O_RDWR | O_NOCTTY);
    if (fd < 0) {
        printf("Unable to open UART com port...\n");
        return NULL;
    }

    // configure UART
    struct termios tty;
    tcgetattr(fd, &tty);
    cfsetospeed(&tty, B115200);
    cfsetispeed(&tty, B115200);
    tty.c_cflag = CS8 | CREAD | CLOCAL;
    tty.c_lflag = 0;
    tty.c_iflag = 0;
    tty.c_oflag = 0;
    tcsetattr(fd, TCSANOW, &tty);

    uint8_t byte;

    while (1) {
        int n = read(fd, &byte, 1);
        if (n <= 0) continue;

        switch (state) {
        case 0:
            if (byte == START_BYTE) {
                buffer[0] = byte;
                idx = 1;
                state = 1;
            }
            break;

        case 1:
            buffer[idx++] = byte;

            if (idx == 14) {
                process_packet(buffer);
                state = 0;
            }
            break;
        }
    }

    close(fd);

    return NULL;
}

int main() {
    pthread_t lcd_thread;
    pthread_t uart_thread;

    if(pthread_create(&lcd_thread, NULL, lcd_main, NULL) != 0){
        printf("Unable to create LCD thread...\n");
        return 0;
    }

    pthread_detach(lcd_thread);

    if(pthread_create(&uart_thread, NULL, uart_main, NULL) != 0){
        printf("Unable to create UART thread...\n");
        return 0;
    }

    pthread_detach(uart_thread);
    
    int led_gpio = 26;
    gpio_request_output(led_gpio);

    while(1){  
        gpio_set(led_gpio, 1);
        printf("Led should be on...\n");
        sleep(5);
        gpio_set(led_gpio, 0);
        printf("Led should be off...\n");
        sleep(5);
    }

    return 0;
}
