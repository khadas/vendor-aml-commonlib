/*
 *
 * Copyright (C) 2021 Amlogic, Inc. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 * This properties module provides a common mechanism to get system configuration.
 *
 *
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "aml_device_property.h"


static char *saved_system_property_file = NULL;
#define DEFAULT_EXPORT_SYSTEM_PROPERTY_FILE "DEVICE_PROPERTYIES_FILE"
#define DEFAULT_SYSTEM_PROPERTY_FILE "/etc/device.properties"
/**
 * @brief AmlDeviceSetPropertyFile
 * @param file system property file name
 * @retrun 0: Success, -1: failed
 *
 * @detail
 *    Figure out the system property file
 */
int AmlDeviceSetPropertyFile(const char* filename) {
  saved_system_property_file = (char*) malloc (strlen(filename) + 1);
  if (NULL == saved_system_property_file)
    return AMLDEVICE_MEMORY_NOT_ENOUGH;

  strcpy(saved_system_property_file, filename);
}

/**
 * @brief AmlDeviceParsePropertyFile
 * @param filename  device property file name
 * @param value output property value
 * @param length the length of value
 * @retrun 0: Success, -1: failed
 *
 * @detail
 *    Get property value base name.
 */
static int AmlDeviceParsePropertyFile(char* filename,
                   const char* valuename,char value[], int length) {
  FILE* properties = fopen(filename, "r");
  if (!properties) {
    //printf("Cannot open file %s\n", filename);
    return AMLDEVICE_OPEN_FILE_FAILED;
  }

  int result = AMLDEVICE_FAILED;
  char* buffer = NULL;
  size_t size = 0;
  // Parse the property file line by line
  while (getline(&buffer, &size, properties) != -1) {
    // Check whether does this line include the specify valuename
    char* remainder = buffer;
    size_t remainder_length = size;
    // Remove space from begin
    while (remainder[0] == ' ' && remainder_length >= 1) {
      remainder ++;
      remainder_length --;
    }

    if (remainder[0] == '#' || remainder_length <= 1)
      continue;
    remainder = strstr(remainder, valuename);
    if (remainder) {
      // Remove the value name
      int len = strlen(valuename);
      remainder +=  len;
      remainder_length -= len;

      int i = 0;
      while ((remainder[0] == ' '|| remainder[0] == '=' )
          && remainder_length >= 1) {
        remainder ++;
        remainder_length --;
      }
      if (remainder_length > 1) {
        if (strlen(remainder) > length) {
          result = AMLDEVICE_OUTPUT_NOT_ENOUGH;
          break;
        }
        // remove ' ', '\n', '\r' from the end of string
        i = strlen(remainder) - 1;
        while(i >= 0 &&
            (' ' == remainder[i] || '\r' == remainder[i] || '\n' == remainder[i])) {
          i --;
        }
        remainder[i+1] = '\0';
        // copy the value
        strcpy (value, remainder);
        result = AMLDEVICE_SUCCESS;
        break;
      }
    }
  }

  if (buffer) free(buffer);
  fclose(properties);

  return result;
}

/**
 * @brief AmlDeviceGetProperty
 * @param name  Property name
 * @param value output property value
 * @param length the length of value
 * @retrun 0: Success, -1: failed
 *
 * @detail
 *    Get property value base name.
 */
int AmlDeviceGetProperty(const char* name, char value[], int length) {
  if (NULL ==  name || NULL == value || length <= 0)
    return AMLDEVICE_PARAMETER_INVALID;
  // Check export environment first
  const char *env = getenv(name);
  if (NULL != env) {
    if ( strlen(env) > length ) {
      return AMLDEVICE_OUTPUT_NOT_ENOUGH;
    }
    strncpy(value, env, length);
    length = strlen(env);
    return AMLDEVICE_SUCCESS;
  }

  // Check property files
  if (saved_system_property_file)
    return AmlDeviceParsePropertyFile(saved_system_property_file, name, value, length);

  char *property_file = getenv(DEFAULT_EXPORT_SYSTEM_PROPERTY_FILE);
  if (NULL != property_file)
    return AmlDeviceParsePropertyFile(property_file, name, value, length);

  return AmlDeviceParsePropertyFile(DEFAULT_SYSTEM_PROPERTY_FILE, name, value, length);
}


