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

