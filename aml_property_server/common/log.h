/*
 * Copyright (C) 2014-present Amlogic, Inc. All rights reserved.
 *
 * All information contained herein is Amlogic confidential.
 *
 * This software is provided to you pursuant to Software License Agreement
 * (SLA) with Amlogic Inc ("Amlogic"). This software may be used
 * only in accordance with the terms of this agreement.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification is strictly prohibited without prior written permission from
 * Amlogic.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#pragma once

namespace Amlogic {
namespace Platform {
typedef enum logLevel {
  LOG_SILENT = 0,
  LOG_FATAL,
  LOG_ERROR,
  LOG_WARN,
  LOG_INFO,
  LOG_DEBUG,
  LOG_VERBOSE,
  LOG_MAX
} LogLevel;

#define LOGV(fmt, arg...)                                                      \
  log_print(LOG_VERBOSE, LOG_TAG, __FILE__, __PRETTY_FUNCTION__, __LINE__,     \
            fmt, ##arg)
#define LOGD(fmt, arg...)                                                      \
  log_print(LOG_DEBUG, LOG_TAG, __FILE__, __PRETTY_FUNCTION__, __LINE__, fmt,  \
            ##arg)
#define LOGI(fmt, arg...)                                                      \
  log_print(LOG_INFO, LOG_TAG, __FILE__, __PRETTY_FUNCTION__, __LINE__, fmt,   \
            ##arg)
#define LOGW(fmt, arg...)                                                      \
  log_print(LOG_WARN, LOG_TAG, __FILE__, __PRETTY_FUNCTION__, __LINE__, fmt,   \
            ##arg)
#define LOGE(fmt, arg...)                                                      \
  log_print(LOG_ERROR, LOG_TAG, __FILE__, __PRETTY_FUNCTION__, __LINE__, fmt,  \
            ##arg)
#define LOGF(fmt, arg...)                                                      \
  log_print(LOG_FATAL, LOG_TAG, __FILE__, __PRETTY_FUNCTION__, __LINE__, fmt,  \
            ##arg)

#define DUMPV(buf, len)                                                        \
  dump_buffer(LOG_VERBOSE, LOG_TAG, __FILE__, __PRETTY_FUNCTION__, __LINE__,   \
              buf, len)
#define DUMPD(buf, len)                                                        \
  dump_buffer(LOG_DEBUG, LOG_TAG, __FILE__, __PRETTY_FUNCTION__, __LINE__,     \
              buf, len)
#define DUMPI(buf, len)                                                        \
  dump_buffer(LOG_INFO, LOG_TAG, __FILE__, __PRETTY_FUNCTION__, __LINE__, buf, \
              len)
#define DUMPW(buf, len)                                                        \
  dump_buffer(LOG_WARN, LOG_TAG, __FILE__, __PRETTY_FUNCTION__, __LINE__, buf, \
              len)
#define DUMPE(buf, len)                                                        \
  dump_buffer(LOG_ERROR, LOG_TAG, __FILE__, __PRETTY_FUNCTION__, __LINE__,     \
              buf, len)
#define DUMPF(buf, len)                                                        \
  dump_buffer(LOG_FATAL, LOG_TAG, __FILE__, __PRETTY_FUNCTION__, __LINE__,     \
              buf, len)

void set_log_level(LogLevel level);
void log_print(LogLevel level, const char *tag, const char *file,
               const char *function, int line, const char *fmt, ...)
    __attribute__((format(printf, 6, 7)));
void dump_buffer(LogLevel level, const char *tag, const char *file,
                 const char *function, int line, const char *buf, int length);
} // namespace Platform
} // namespace Amlogic

