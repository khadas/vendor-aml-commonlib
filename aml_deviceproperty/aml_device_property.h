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

#ifndef _AML_DEVICE_PROPERTY_H_
#define _AML_DEVICE_PROPERTY_H_
#ifdef __cplusplus
extern "C" {
#endif


// return value definitions
#define AMLDEVICE_SUCCESS     0
#define AMLDEVICE_FAILED      -1

#define AMLDEVICE_PARAMETER_INVALID             0x1000
#define AMLDEVICE_MEMORY_NOT_ENOUGH             0x1001
#define AMLDEVICE_OPEN_FILE_FAILED              0x1002
#define AMLDEVICE_OUTPUT_NOT_ENOUGH             0x1003

/**
 * @brief AmlDeviceSetPropertyFile
 * @param file system property file name
 * @retrun 0: Success, -1: failed
 *
 * @detail
 *    Figure out the system property file
 */
int AmlDeviceSetPropertyFile(const char* filename);

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
int AmlDeviceGetProperty(const char* name, char value[], int length);

#ifdef __cplusplus
}  // extern "C"
#endif

#endif // _AML_DEVICE_PROPERTY_H_

