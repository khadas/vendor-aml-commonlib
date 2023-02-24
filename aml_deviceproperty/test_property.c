// Copyright (C) 2020 Amlogic, Inc. All rights reserved.
//
// All information contained herein is Amlogic confidential.
//
// This software is provided to you pursuant to Software License
// Agreement (SLA) with Amlogic Inc ("Amlogic"). This software may be
// used only in accordance with the terms of this agreement.
//
// Redistribution and use in source and binary forms, with or without
// modification is strictly prohibited without prior written permission
// from Amlogic.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
// "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
// LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
// A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
// OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
// SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
// LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
// DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
// THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

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


