/*
 * trace.c
 *
 *  Created on: 2019年3月20日
 *      Author: tao
 */

#include <stdio.h>
#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <pthread.h>
#include <stdatomic.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <inttypes.h>
#include <sys/types.h>

#define ATRACE_MESSAGE_LENGTH 1024

#define WRITE_MSG(format_begin, format_end, name, value) { \
    char buf[ATRACE_MESSAGE_LENGTH]; \
    int pid = getpid(); \
    int len = snprintf(buf, sizeof(buf), format_begin "%s" format_end, pid, \
        name, value); \
    if (len >= (int) sizeof(buf)) { \
        /* Given the sizeof(buf), and all of the current format buffers, \
         * it is impossible for name_len to be < 0 if len >= sizeof(buf). */ \
        int name_len = strlen(name) - (len - sizeof(buf)) - 1; \
        /* Truncate the name to make the message fit. */ \
        len = snprintf(buf, sizeof(buf), format_begin "%.*s" format_end, pid, \
            name_len, name, value); \
    } \
    write(atrace_marker_fd, buf, len); \
}

#define atomic_store_explicit(PTR, VAL, MO)             \
  __extension__                             \
  ({                                    \
    __auto_type __atomic_store_ptr = (PTR);             \
    __typeof__ (*__atomic_store_ptr) __atomic_store_tmp = (VAL);    \
    __atomic_store (__atomic_store_ptr, &__atomic_store_tmp, (MO)); \
  })

int                     atrace_marker_fd     = -1;
atomic_bool             atrace_is_ready      = ATOMIC_VAR_INIT(false);
static pthread_once_t   atrace_once_control  = PTHREAD_ONCE_INIT;

static void atrace_init_once()
{
    atrace_marker_fd = open("/sys/kernel/debug/tracing/trace_marker", O_WRONLY | O_CLOEXEC);
    if (atrace_marker_fd == -1) {
        goto done;
    }

done:
    atomic_store_explicit(&atrace_is_ready, true, memory_order_release);
}

void __attribute__((constructor)) atrace_setup()
{
    pthread_once(&atrace_once_control, atrace_init_once);
}

void atrace_begin_body(const char* name)
{
    WRITE_MSG("B|%d|", "%s", name, "");
}

void atrace_end_body()
{
    WRITE_MSG("E|%d", "%s", "", "");
}

void atrace_async_begin_body(const char* name, int32_t cookie)
{
    WRITE_MSG("S|%d|", "|%" PRId32, name, cookie);
}

void atrace_async_end_body(const char* name, int32_t cookie)
{
    WRITE_MSG("F|%d|", "|%" PRId32, name, cookie);
}

void atrace_int_body(const char* name, int32_t value)
{
    WRITE_MSG("C|%d|", "|%" PRId32, name, value);
}

void atrace_int64_body(const char* name, int64_t value)
{
    WRITE_MSG("C|%d|", "|%" PRId64, name, value);
}

void atrace_uint_body(const char* name, uint32_t value)
{
    WRITE_MSG("C|%d|", "|%" PRIu32, name, value);
}

void atrace_uint64_body(const char* name, uint64_t value)
{
    WRITE_MSG("C|%d|", "|%" PRIu64, name, value);
}



