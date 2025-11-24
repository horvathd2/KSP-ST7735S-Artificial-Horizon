#include "spi.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <linux/spi/spidev.h>
#include <sys/ioctl.h>

int spi_init(spi_device_t *dev,
             const char *device_path,
             uint8_t mode,
             uint32_t speed_hz,
             uint8_t bits_per_word)
{
    memset(dev, 0, sizeof(*dev));

    dev->fd = open(device_path, O_RDWR);
    if (dev->fd < 0) {
        perror("SPI: cannot open device");
        return -1;
    }

    dev->mode = mode;
    dev->speed_hz = speed_hz;
    dev->bits_per_word = bits_per_word;
    dev->delay_us = 0;

    // Mode
    if (ioctl(dev->fd, SPI_IOC_WR_MODE, &dev->mode) == -1) {
        perror("SPI: cannot set mode");
        return -1;
    }

    // Bits per word
    if (ioctl(dev->fd, SPI_IOC_WR_BITS_PER_WORD, &dev->bits_per_word) == -1) {
        perror("SPI: cannot set bits per word");
        return -1;
    }

    // Speed
    if (ioctl(dev->fd, SPI_IOC_WR_MAX_SPEED_HZ, &dev->speed_hz) == -1) {
        perror("SPI: cannot set speed");
        return -1;
    }

    printf("SPI initialized: %s\n", device_path);
    printf("  Mode: %d\n", dev->mode);
    printf("  Speed: %u Hz\n", dev->speed_hz);
    printf("  Bits: %u\n", dev->bits_per_word);

    return 0;
}

int spi_transfer(spi_device_t *dev,
                 const uint8_t *tx,
                 uint8_t *rx,
                 size_t len)
{
    struct spi_ioc_transfer tr = {
        .tx_buf = (unsigned long)tx,
        .rx_buf = (unsigned long)rx,
        .len = len,
        .delay_usecs = dev->delay_us,
        .speed_hz = dev->speed_hz,
        .bits_per_word = dev->bits_per_word,
    };

    int ret = ioctl(dev->fd, SPI_IOC_MESSAGE(1), &tr);
    if (ret < 1) {
        perror("SPI: transfer failed");
        return -1;
    }

    return 0;
}

int spi_write(spi_device_t *dev,
              const uint8_t *tx,
              size_t len)
{
    return spi_transfer(dev, tx, NULL, len);
}

#define CHUNK 4096

void spi_write_chunked(spi_device_t *dev, const uint8_t *data, size_t len) {
    size_t pos = 0;

    while (pos < len) {
        size_t n = len - pos;
        if (n > CHUNK) n = CHUNK;

        int ret = write(dev->fd, data + pos, n);
        if (ret < 0) {
            perror("spi write");
            return;
        }

        pos += n;
    }
}

void spi_close(spi_device_t *dev)
{
    if (dev->fd >= 0)
        close(dev->fd);
    dev->fd = -1;
}
