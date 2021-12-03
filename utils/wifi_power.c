#include <fcntl.h>
#include <asm/ioctl.h>
#include <stdlib.h>
#include <stdio.h>

#define USB_POWER_UP    _IO('m',1)
#define USB_POWER_DOWN  _IO('m',2)
#define SDIO_POWER_UP    _IO('m', 3)
#define SDIO_POWER_DOWN  _IO('m', 4)
#define SDIO_POWER_DOWN  _IO('m', 4)
#define SDIO_GET_DEV_TYPE  _IO('m', 5)

char dev_type[128] = {0};

void set_wifi_power(int on)
{
    int fd;

    fd = open("/dev/wifi_power", O_RDWR);
    if (fd != -1) {
        if (on == 0) {
            if (ioctl(fd,SDIO_POWER_DOWN)<0) {
                if (ioctl(fd,USB_POWER_DOWN) < 0) {
                    printf("Set Wi-Fi power down error!!!\n");
                    close(fd);
                    return;
                }
            }
        } else if (on == 1) {
            if (ioctl(fd,SDIO_POWER_UP) < 0) {
                if (ioctl(fd,USB_POWER_UP) < 0) {
                    printf("Set Wi-Fi power up error!!!\n");
                    close(fd);
                    return;
                }
            }
        } else if (on == 2) {
            if (ioctl(fd, SDIO_GET_DEV_TYPE, dev_type) < 0) {
                printf("Set Wi-Fi power up error!!!\n");
                close(fd);
                return;
            } else
                printf("inf=%s\n", dev_type);
       }
    } else {
        printf("Device open failed !!!\n");
    }

    close(fd);
    return;
}

int main(int argc, char *argv[])
{
    long value = 0;
    if (argc != 2) {
        printf("wrong number of arguments\n");
        return -1;
    }
    value = strtol(argv[1], NULL, 10);
    set_wifi_power(value);
    return 0;
}

