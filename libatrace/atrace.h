/*
 * atrace.h
 *
 *  Created on: 2019年3月20日
 *      Author: tao
 */

#ifndef SRC_ATRACE_H_
#define SRC_ATRACE_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <inttypes.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <sys/cdefs.h>
#include <sys/types.h>
#include <unistd.h>



#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wnested-externs"
#pragma GCC diagnostic ignored "-Wstrict-prototypes"


#define ATRACE_BEGIN(name) atrace_begin(name)
static inline void atrace_begin(const char* name)
{
    void atrace_begin_body(const char*);
    atrace_begin_body(name);
}

#define ATRACE_END() atrace_end()
static inline void atrace_end()
{
    void atrace_end_body();
    atrace_end_body();
}

#define ATRACE_ASYNC_BEGIN(name, cookie) \
    atrace_async_begin(name, cookie)
static inline void atrace_async_begin(const char* name,
        int32_t cookie)
{
    void atrace_async_begin_body(const char*, int32_t);
    atrace_async_begin_body(name, cookie);
}


#define ATRACE_ASYNC_END(name, cookie) atrace_async_end(name, cookie)
static inline void atrace_async_end(const char* name, int32_t cookie)
{
    void atrace_async_end_body(const char*, int32_t);
    atrace_async_end_body(name, cookie);
}

#define ATRACE_INT(name, value) atrace_int(name, value)
static inline void atrace_int(const char* name, int32_t value)
{
    void atrace_int_body(const char*, int32_t);
    atrace_int_body(name, value);
}

#define ATRACE_INT64(name, value) atrace_int64(name, value)
static inline void atrace_int64(const char* name, int64_t value)
{
    void atrace_int64_body(const char*, int64_t);
    atrace_int64_body(name, value);
}

#define ATRACE_UINT(name, value) atrace_uint(name, value)
static inline void atrace_uint(const char* name, uint32_t value)
{
    void atrace_uint_body(const char*, uint32_t);
    atrace_uint_body(name, value);
}

#define ATRACE_UINT64(name, value) atrace_uint64(name, value)
static inline void atrace_uint64(const char* name, uint64_t value)
{
    void atrace_uint64_body(const char*, uint64_t);
    atrace_uint64_body(name, value);
}

#pragma GCC diagnostic pop

#ifdef __cplusplus
}

#define ATRACE_NAME(name) ScopedTrace ___tracer_##name(name)
#define ATRACE_NAME_ASYNC(name) ScopedTraceAsync ___tracer_async_##name(name, true)
#define ATRACE_NAME_ASYNC2(name) ScopedTraceAsync ___tracer_async_##name(#name, true)
#define ATRACE_CALL() ATRACE_NAME(__FUNCTION__)

class ScopedTraceAsync {
public:
    inline ScopedTraceAsync(const char *name, bool enable) {
        cookie = random();
        tag = strdup(name);
        enable_ = enable;
        if (enable)
            ATRACE_ASYNC_BEGIN( tag, cookie);
    }

    inline ~ScopedTraceAsync() {
        if (enable_)
            ATRACE_ASYNC_END(tag, cookie);
        free(tag);
    }
private:
    char *tag;
    int32_t cookie;
    bool enable_;
};

class ScopedTrace {
public:
    inline ScopedTrace(const char* name) {
        atrace_begin(name);
    }

    inline ~ScopedTrace() {
        atrace_end();
    }
};

#endif
#endif /* SRC_ATRACE_H_ */
