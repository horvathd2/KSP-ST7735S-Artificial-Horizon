#ifndef __SPI_H_
#define __SPI_H_

#include <stdint.h>
#include <stddef.h>

typedef struct {
    int fd;
    uint8_t mode;
    uint8_t bits_per_word;
    uint32_t speed_hz;
    uint16_t delay_us;
} spi_device_t;

/**
 * Initialize SPI device.
 *
 * Example dev path: "/dev/spidev0.0"
 */
int spi_init(spi_device_t *dev,
             const char *device_path,
             uint8_t mode,
             uint32_t speed_hz,
             uint8_t bits_per_word);

/**
 * Full duplex transfer
 */
int spi_transfer(spi_device_t *dev,
                 const uint8_t *tx,
                 uint8_t *rx,
                 size_t len);

/**
 * Simple write (half-duplex)
 */
int spi_write(spi_device_t *dev,
              const uint8_t *tx,
              size_t len);

/**
 * Writing SPI message split into chunks
 */
void spi_write_chunked(spi_device_t *dev, 
                       const uint8_t *data, 
                       size_t len);

/**
 * Close device
 */
void spi_close(spi_device_t *dev);


#endif