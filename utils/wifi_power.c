#include <dirent.h>
#include <fcntl.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <unistd.h>

#define USB_POWER_UP _IO('m', 1)
#define USB_POWER_DOWN _IO('m', 2)
#define SDIO_POWER_UP _IO('m', 3)
#define SDIO_POWER_DOWN _IO('m', 4)
#define SDIO_POWER_DOWN _IO('m', 4)
#define SDIO_GET_DEV_TYPE _IO('m', 5)

char dev_type[128] = {0};

bool set_wifi_power(int on) {
  int fd;
  bool ret = true;

  fd = open("/dev/wifi_power", O_RDWR);
  if (fd < 0) {
    fprintf(stderr, "Device open failed !!!\n");
    return false;
  }

  switch (on) {
  case 0: {
    if (ioctl(fd, SDIO_POWER_DOWN) < 0) {
      if (ioctl(fd, USB_POWER_DOWN) < 0) {
        fprintf(stderr, "Set Wi-Fi power down error!!!\n");
        close(fd);
        ret = false;
      }
    }
  } break;
  case 1: {
    if (ioctl(fd, SDIO_POWER_UP) < 0) {
      if (ioctl(fd, USB_POWER_UP) < 0) {
        fprintf(stderr, "Set Wi-Fi power up error!!!\n");
        close(fd);
        ret = false;
      }
    }
  } break;
  case 2: {
    if (ioctl(fd, SDIO_GET_DEV_TYPE, dev_type) < 0) {
      fprintf(stderr, "Failed to get dev type!!!\n");
      close(fd);
      ret = false;
    } else
      fprintf(stdout, "inf=%s\n", dev_type);
  } break;
  default:
    fprintf(stderr, "Invalid command %d\n", on);
    ret = false;
  }
  close(fd);

  return ret;
}

int main(int argc, char *argv[]) {
  long value = 0;
  if (argc < 2) {
    fprintf(stderr, "wrong number of arguments\n");
    return -1;
  }
  value = strtol(argv[1], NULL, 10);
  if (!set_wifi_power(value)) {
    return -1;
  }

  if (value == 1 && argc > 2) {
    const char *interface_name = argv[2];

    int wait_time = 100;
    char fpath[64];
    snprintf(fpath, sizeof(fpath), "/sys/class/net/%s", interface_name);
    do {
      if (access(fpath, 0) == 0)
        break;
      usleep(50000);
    } while (wait_time--);
    if (wait_time < 0) {
      fprintf(stderr, "wait for interface [%s] timeout\n", interface_name);
      return -1;
    }
  }

  return 0;
}

