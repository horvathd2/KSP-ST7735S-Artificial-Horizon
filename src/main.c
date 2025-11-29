#include "st7735s.h"
#include "gpio.h"
#include "horizon.h"
#include <stdio.h>
#include <unistd.h>
#include <malloc.h>
#include <pthread.h>
#include <stdint.h>
#include <fcntl.h>
#include <termios.h>
#include <string.h>
#include <time.h>
#include <math.h>

#define PACKET_SIZE     7
#define START_BYTE      0xAA

typedef struct __attribute__((packed)){
    uint8_t start_byte;
    int16_t pitch;
    int16_t roll;
    int16_t yaw;
}uart_packet;

static uint8_t receive_buffer[PACKET_SIZE];
static int state = 0;
static int idx = 0;
static uart_packet uartMsg;

pthread_mutex_t uart_packet_mutex = PTHREAD_MUTEX_INITIALIZER;

uint64_t get_ticks_us()
{
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint64_t)ts.tv_sec * 1000000ULL + (ts.tv_nsec / 1000);
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
            /**
             * Check if the first received byte is the starting byte (0xAA)
             * If it is, switch state to 1 so that every reading cycle it processes
             * The remaining bytes in the packet. Once index == PACKET_SIZE (7 bytes)
             * Copy from receive buffer into mutex packet and reset.
             */
            if (byte == START_BYTE) {
                receive_buffer[0] = byte;
                idx = 1;
                state = 1;
            }
            break;

        case 1:
            receive_buffer[idx++] = byte;

            if (idx == PACKET_SIZE) {
                pthread_mutex_lock(&uart_packet_mutex);
                memcpy(&uartMsg, receive_buffer, PACKET_SIZE);
                pthread_mutex_unlock(&uart_packet_mutex);
                state = 0;
            }
            break;
        }
    }

    close(fd);

    return NULL;
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

    st7735s_fill_screen(&horizon_lcd, 0x0000);

    int16_t pitch, roll, yaw;

    while(1){
        pthread_mutex_lock(&uart_packet_mutex);
        pitch = uartMsg.pitch;
        roll = uartMsg.roll;
        yaw = uartMsg.yaw;
        pthread_mutex_unlock(&uart_packet_mutex);

        // int pitch = 300 * sin(get_ticks_us() / 10.0);
        // int roll = 0;//4 * fsin(HAL_GetTick() / 12.0);
        // int yaw = fmod(get_ticks_us() / 20.0, 360); 

        draw_navball(pitch, roll, yaw);
        framebuffer_draw_circle(radius+1, cx, cy, 0x07E0);
        st7735s_push_framebuffer(&horizon_lcd, horizon_get_framebuffer(), FB_WIDTH, FB_HEIGHT);
    }

    return NULL;
}

int main() {
    pthread_t uart_thread;
    pthread_t lcd_thread;

    if(pthread_create(&uart_thread, NULL, uart_main, NULL) != 0){
        printf("Unable to create UART thread...\n");
        return 0;
    }

    pthread_detach(uart_thread);

    if(pthread_create(&lcd_thread, NULL, lcd_main, NULL) != 0){
        printf("Unable to create LCD thread...\n");
        return 0;
    }

    pthread_detach(lcd_thread);
    
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
