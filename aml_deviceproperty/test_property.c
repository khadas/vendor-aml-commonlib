#include <stdio.h>
#include "aml_device_property.h"


int main(int argc, char *argv[]) {
  int ret;
  char value[50];
  int length = 50;
  printf("argc = %d\n", argc);
  if (argc == 2 && argv[1] != NULL) {
    ret = AmlDeviceGetProperty(argv[1], value, length);
    printf("get ret = %x, %s = \"%s\"\n", ret,argv[1], ret?"NULL":value);
    int size = strlen(value);
    int i;
    for (i=0; i < size; i++)
      printf("size = %d, %d: %c (%d)\n", size, i, value[i], (int)value[i]);

    return 0;
  }
  //AmlDeviceSetPropertyFile("/etc/device.properties");
  printf("test property function\n");
  ret = AmlDeviceGetProperty("MODEL_NAME", value, length);
  printf("test ret = %x, result = %s\n", ret, ret?"NULL":value);
  ret = AmlDeviceGetProperty("BRAND_NAME", value, length);
  printf("test ret = %x, result = %s\n", ret, ret?"NULL":value);
  ret = AmlDeviceGetProperty("CERT_SCOPE", value, length);
  printf("test ret = %x, result = %s\n", ret, ret?"NULL":value);
  ret = AmlDeviceGetProperty("YOUTUBE_DEVICE_TYPE", value, length);
  printf("test ret = %x, result = %s\n", ret, ret?"NULL":value);
  ret = AmlDeviceGetProperty("MODULE_NAME4", value, length);
  printf("test ret = %x, result = %s\n", ret, ret?"NULL":value);
  ret = AmlDeviceGetProperty("MODULE_NAME5", value, length);
  printf("test ret = %x, result = %s\n", ret, ret?"NULL":value);
  return 0;
}


