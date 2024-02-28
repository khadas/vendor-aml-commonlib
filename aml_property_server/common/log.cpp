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

#include <ctype.h>
#include <libgen.h>
#include <stdarg.h>
#include <stdio.h>
#include <sys/time.h>
#include <time.h>
#include <unistd.h>

#include "log.h"

namespace Amlogic {
namespace Platform {

#define RESET "\x1b[0m"
#define BLACK "\x1b[30m"
#define RED "\x1b[31m"
#define GREEN "\x1b[32m"
#define YELLOW "\x1b[33m"
#define BLUE "\x1b[34m"
#define MAGENTA "\x1b[35m"
#define CYAN "\x1b[36m"
#define WHITE "\x1b[37m"
#define BGBLACK "\x1b[40m"
#define BGRED "\x1b[41m"
#define BGGREEN "\x1b[42m"
#define BGYELLOW "\x1b[43m"
#define BGBLUE "\x1b[44m"
#define BGMAGENTA "\x1b[45m"
#define BGCYAN "\x1b[46m"
#define BGWHITE "\x1b[47m"

static LogLevel gs_log_level = LOG_ERROR;
void set_log_level(LogLevel level) { gs_log_level = level; }

const char *get_level_name(int level) {
  const char *name = "[UNKNOWN]";
  switch (level) {
  case LOG_VERBOSE:
    name = WHITE "[VERBOSE]" RESET;
    break;
  case LOG_DEBUG:
    name = BLUE "[DEBUG]" RESET;
    break;
  case LOG_INFO:
    name = GREEN "[INFO]" RESET;
    break;
  case LOG_WARN:
    name = YELLOW "[WARN]" RESET;
    break;
  case LOG_ERROR:
    name = RED "[ERROR]" RESET;
    break;
  case LOG_FATAL:
    name = YELLOW BGRED "[FATAL]" RESET;
    break;
  default:
    break;
  }
  return name;
}

static void print_header(LogLevel level, const char *tag, const char *file,
                         const char *function, int line) {
  struct tm *tm_info;
  struct timeval tv;
  gettimeofday(&tv, NULL);
  tm_info = localtime(&tv.tv_sec);
  fprintf(
      stderr, WHITE BGBLUE "[%04d-%02d-%02d %02d:%02d:%02d.%03ld]" RESET " ",
      (1900 + tm_info->tm_year), tm_info->tm_mon + 1, tm_info->tm_mday,
      tm_info->tm_hour, tm_info->tm_min, tm_info->tm_sec, tv.tv_usec / 1000);
  fprintf(stderr, "%10s %16s[%d]", get_level_name(level), tag, gettid());
  fprintf(stderr, "[%s:%d - %s] ", basename((char *)file), line, function);
}

void log_print(LogLevel level, const char *tag, const char *file,
               const char *function, int line, const char *fmt, ...) {
  if (gs_log_level >= level) {
    print_header(level, tag, file, function, line);
    int size = 0;
    char *p = NULL;
    va_list ap;
    va_start(ap, fmt);
    size = vsnprintf(p, size, fmt, ap);
    va_end(ap);
    if (size < 0)
      return;

    size++; /* For '\\0' */
    p = new char[size];
    if (p == NULL)
      return;

    va_start(ap, fmt);
    size = vsnprintf(p, size, fmt, ap);
    va_end(ap);
    if (size < 0) {
      delete[] p;
      return;
    }

    fprintf(stderr, "%s", p);
    delete[] p;
  }
}

void dump_buffer(LogLevel level, const char *tag, const char *file,
                 const char *function, int line, const char *buf, int length) {
  if (gs_log_level >= level) {
    print_header(level, tag, file, function, line);
    fprintf(stderr, "[%p]:%d\n", buf, length);
    int i = 0;
    for (; i < length; i += 16) {
      fprintf(stderr, "%08x: ", i);
      for (int j = 0; j < 16; j++) {
        if (i + j < length) {
          fprintf(stderr, "%02x ", buf[i + j]);
        } else {
          fprintf(stderr, "   ");
        }
      }
      fprintf(stderr, " ");
      for (int j = 0; j < 16; j++) {
        if (i + j < length) {
          fprintf(stderr, "%c", isprint(buf[i + j]) ? buf[i + j] : '.');
        } else {
          break;
        }
      }
      fprintf(stderr, "\n");
    }
  }
}
} // namespace Platform
} // namespace Amlogic
