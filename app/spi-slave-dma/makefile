# Makefile for SPI Slave DMA Application

CC ?= aarch64-nuvoton-linux-gcc
CFLAGS ?= -Wall -O2
LDFLAGS ?= 

TARGET = spi_slave_test

SRCS = main.c
OBJS = $(SRCS:.c=.o)

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(TARGET) $(OBJS)

.PHONY: all clean