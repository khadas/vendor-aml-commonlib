#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <linux/input.h>
#include <linux/uinput.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>

void simulate_key(int fd, int kval)
{
  struct input_event event;
  int ret;

  gettimeofday(&event.time,0);
  event.type = EV_KEY;
  event.code = kval;
  event.value = 1;
  ret = write(fd, &event, sizeof(event));

  event.type = EV_SYN;
  event.code = SYN_REPORT;
  event.value = 0;
  ret = write(fd, &event, sizeof(event));

  memset(&event, 0, sizeof(event));
  gettimeofday(&event.time, NULL);
  event.type = EV_KEY;
  event.code = kval;
  event.value = 0;
  ret = write(fd, &event, sizeof(event));

  event.type = EV_SYN;
  event.code = SYN_REPORT;
  event.value = 0;
  ret = write(fd, &event, sizeof(event));
}

int main(int argc, char *argv[])
{
  unsigned int keycode;
  int fd_kbd;

  if (argc != 2) {
    printf("Usage: simulate_key keycode\n");
    return -1;
  }

  if (strlen(IR_REMOTE_DEVICE)) {
    //printf("IR_REMOTE_DEVICE:%s\n", IR_REMOTE_DEVICE);
    fd_kbd = open(IR_REMOTE_DEVICE, O_RDWR);
  } else {
    fd_kbd = open("/dev/input/event0", O_RDWR);
  }
  if (fd_kbd <= 0) {
    printf("error open keyboard\n");
    return -1;
  }

  keycode = strtoul(argv[1], NULL, 0);
  printf("send simulate keycode: %d (0x%x)!\n", keycode, keycode);
  simulate_key(fd_kbd, keycode);

  close(fd_kbd);
}
