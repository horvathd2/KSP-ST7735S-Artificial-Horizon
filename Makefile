CC = arm-linux-gnueabihf-gcc
CFLAGS = -O2 -Wall

SRC = src/main.c src/spi.c src/st7735s.c src/gpio.c src/horizon.c src/navball_texture_160_80.c src/navball_texture_256_128.c
OBJ = $(SRC:.c=.o)

all: lcd_app

LCD_APP_LDFLAGS = -lgpiod -lpthread -lm

lcd_app: $(OBJ)
	$(CC) $(OBJ) $(LCD_APP_LDFLAGS) -o lcd_app
clean:
	rm -f $(OBJ) lcd_app

