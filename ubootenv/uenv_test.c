#include "ubootenv.h"
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

void print_help(char *argv[]) {
  printf("Usage:%s dump|set|get [prop] [value]\n\n", argv[0]);
  exit(1);
}

int main(int argc, char *argv[]) {
  const char *cmd = argv[1];
  const char *prop = argv[2];
  const char *value = argv[3];

  if ((argc > 4) || (argc < 2)) {
    print_help(argv);
  }
  bootenv_init();
  if (!strcmp("dump", cmd)) {
    bootenv_print();
  } else if (!strcmp("get", cmd) && argc > 2) {
    value = bootenv_get(prop);
    printf("key: [%s]\n", prop);
    printf("value:[%s]\n", value);
  } else if (!strcmp("set", cmd) && argc > 3) {
    bootenv_update(prop, value);
  } else {
    print_help(argv);
  }

  return 0;
}
