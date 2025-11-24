CC = arm-linux-gnueabihf-gcc
CFLAGS = -O2 -Wall

SRC = src/main.c src/spi.c src/st7735s.c src/gpio.c
OBJ = $(SRC:.c=.o)

all: lcd_app

LCD_APP_LDFLAGS = -lgpiod -lpthread

lcd_app: $(OBJ)
	$(CC) $(OBJ) $(LCD_APP_LDFLAGS) -o lcd_app
clean:
	rm -f $(OBJ) lcd_app

