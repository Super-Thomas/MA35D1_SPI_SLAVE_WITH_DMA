#include <stdint.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <linux/types.h>
#include <linux/spi/spidev.h>

static const char *device = "/dev/spidev1.0"; 
static uint8_t mode = 0;        /* SPI_MODE_0 */
static uint8_t bits = 8;        /* 8-bit transfer (Hardware FIFO is automatically configured as 32x8) */
static uint32_t speed = 10000000; 

#define BUFFER_SIZE 100 

int main(int argc, char *argv[])
{
    int fd;
    int ret;
    uint8_t *tx_buf;
    uint8_t *rx_buf;
    int i;

    /* Allocate memory aligned to Page Boundary for DMA transfer */
    posix_memalign((void **)&tx_buf, getpagesize(), BUFFER_SIZE);
    posix_memalign((void **)&rx_buf, getpagesize(), BUFFER_SIZE);

    memset(tx_buf, 0x55, BUFFER_SIZE); // Transmission test data
    memset(rx_buf, 0x00, BUFFER_SIZE); // Initialize reception buffer

    fd = open(device, O_RDWR);
    if (fd < 0) {
        perror("Failed to open SPI2 device");
        exit(1);
    }

    ioctl(fd, SPI_IOC_WR_MODE, &mode);
    ioctl(fd, SPI_IOC_WR_BITS_PER_WORD, &bits);
    ioctl(fd, SPI_IOC_WR_MAX_SPEED_HZ, &speed);

    struct spi_ioc_transfer tr = {
        .tx_buf = (unsigned long)tx_buf,
        .rx_buf = (unsigned long)rx_buf,
        .len = BUFFER_SIZE,
        .delay_usecs = 0,
        .speed_hz = speed,
        .bits_per_word = bits,
    };

    printf("SPI2 (PL.0~PL.3 pins) Slave waiting... \n");

    /* * Since the data size (4096) exceeds the driver threshold, the kernel internally
     * enables SPIx_PDMACTL (RX/TX PDMA EN) confirmed in the TRM and performs a DMA burst transfer.
     */
    ret = ioctl(fd, SPI_IOC_MESSAGE(1), &tr);
    if (ret < 1) {
        perror("SPI2 reception error");
    } else {
        printf("Reception successful! (Size: %d bytes)\n", ret);
        printf("Received data check: \n");
        for (i = 0; i < ret; i++) {
            printf("0x%02X ", rx_buf[i]);
            if ((i + 1) % 10 == 0) {
                printf("\n");
            }
        }
        printf("\n");
    }
    
    free(tx_buf);
    free(rx_buf);
    close(fd);

    return 0;
}
