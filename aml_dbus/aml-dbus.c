/*
 * Copyright (C) 2014-2019 Amlogic, Inc. All rights reserved.
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

#include "aml-dbus.h"
#include <errno.h>
#include <limits.h>
#include <poll.h>
#include <string.h>
#include <sys/queue.h>
#include <time.h>
#include <unistd.h>

struct delay_task {
  struct delay_task *next;
  void (*cb)(void *);
  void *param;
  sd_event_source *src;
  struct aml_dbus *ambus;
  uint32_t delay_ms;
};

struct aml_dbus {
  sd_bus *bus;
  sd_event *event;
  pthread_mutex_t lock;
  pthread_cond_t cond;
  pthread_t dispatch_thread;
  int msgpipe[2];
  struct delay_task *free_task;
};

static int msgpipe_handler(sd_event_source *s, int fd, uint32_t revents, void *userdata) {
  void *args[2];
  int len = read(fd, &args, sizeof(args));
  if (len != sizeof(args)) {
    AMBUS_LOGW("read %d bytes return %d, errno %d %s", (int)sizeof(args), len, errno, strerror(errno));
  } else
    ((void (*)(void *))args[0])(args[1]);
  return 0;
}

int ambus_post_task(struct aml_dbus *ambus, void (*cb)(void *), void *param) {
  void *args[] = {cb, param};
  int len = write(ambus->msgpipe[1], args, sizeof(args));
  if (len != sizeof(args)) {
    AMBUS_LOGW("write %d bytes return %d, errno %d %s", (int)sizeof(args), len, errno, strerror(errno));
    return -1;
  }
  return 0;
}

static int delay_task_timeout(sd_event_source *s, uint64_t usec, void *userdata) {
  struct delay_task *task = (struct delay_task *)userdata;
  struct aml_dbus *ambus = task->ambus;
  pthread_mutex_lock(&ambus->lock);
  task->next = ambus->free_task;
  ambus->free_task = task;
  pthread_mutex_unlock(&ambus->lock);
  task->cb(task->param);
  return 0;
}

static void delay_task_settimer(void *param) {
  struct delay_task *task = (struct delay_task *)param;
  struct aml_dbus *ambus = task->ambus;
  uint64_t timeout;
  int r = sd_event_now(ambus->event, CLOCK_MONOTONIC, &timeout);
  timeout += task->delay_ms * 1000LL;
  if (task->src)
    r = sd_event_source_set_time(task->src, timeout);
  else
    r = sd_event_add_time(ambus->event, &task->src, CLOCK_MONOTONIC, timeout, 1000LL * 10, delay_task_timeout, task);
  if (r >= 0)
    sd_event_source_set_enabled(task->src, SD_EVENT_ONESHOT);
}

int ambus_post_delay_task(struct aml_dbus *ambus, uint32_t delay_ms, void (*cb)(void *), void *param) {
  if (delay_ms == 0)
    return ambus_post_task(ambus, cb, param);
  pthread_mutex_lock(&ambus->lock);
  struct delay_task *task = ambus->free_task;
  if (task)
    ambus->free_task = task->next;
  else {
    task = calloc(1, sizeof(*task));
    task->ambus = ambus;
  }
  pthread_mutex_unlock(&ambus->lock);
  task->cb = cb;
  task->param = param;
  task->delay_ms = delay_ms;
  return ambus_post_task(ambus, delay_task_settimer, task);
}

void ambus_free(struct aml_dbus *ambus) {
  if (!ambus_in_dispatch_thread(ambus)) {
    POST_AND_WAIT_DISPATCH(ambus, ambus_free(ambus));
    pthread_join(ambus->dispatch_thread, NULL);
    free(ambus);
  } else {
    if (ambus->event) {
      sd_event_exit(ambus->event, 0);
      sd_event_unref(ambus->event);
    }
    if (ambus->bus)
      sd_bus_unref(ambus->bus);
    if (ambus->msgpipe[0])
      close(ambus->msgpipe[0]);
    if (ambus->msgpipe[1])
      close(ambus->msgpipe[1]);
  }
}

struct aml_dbus *ambus_new(const char *address, int mode) {
  struct aml_dbus *ambus = calloc(1, sizeof(*ambus));
  ambus->dispatch_thread = pthread_self();
  int r;
  if (address == AMBUS_DEFAULT_SYSTEM || address == NULL) {
    AMBUS_CHECK(r, sd_event_default(&ambus->event), goto fail);
    AMBUS_CHECK(r, sd_bus_default_system(&ambus->bus), goto fail);
  } else if (address == AMBUS_DEFAULT_USER) {
    AMBUS_CHECK(r, sd_event_default(&ambus->event), goto fail);
    AMBUS_CHECK(r, sd_bus_default_user(&ambus->bus), goto fail);
  } else {
    AMBUS_CHECK(r, sd_event_new(&ambus->event), goto fail);
    if (address != AMBUS_DEFAULT_NONE) {
      AMBUS_CHECK(r, sd_bus_new(&ambus->bus), goto fail);
      AMBUS_CHECK(r, sd_bus_set_address(ambus->bus, address), goto fail);
      AMBUS_CHECK(r, sd_bus_start(ambus->bus), goto fail);
    }
  }
  pthread_mutex_init(&ambus->lock, NULL);
  pthread_cond_init(&ambus->cond, NULL);
  if (ambus->bus) {
    AMBUS_CHECK(r, sd_bus_attach_event(ambus->bus, ambus->event, 0), goto fail);
  }
  AMBUS_CHECK(r, pipe(ambus->msgpipe), goto fail);
  AMBUS_CHECK(r, sd_event_add_io(ambus->event, NULL, ambus->msgpipe[0], POLLIN, msgpipe_handler, ambus), goto fail);
  return ambus;
fail:
  ambus_free(ambus);
  return NULL;
}

static pthread_mutex_t global_ambus_mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t global_ambus_cond = PTHREAD_COND_INITIALIZER;
static void *ambus_dispatcher(void *param) {
  struct aml_dbus *ambus = ambus_new(NULL, 0);
  pthread_mutex_lock(&global_ambus_mutex);
  *(struct aml_dbus **)param = ambus;
  pthread_cond_broadcast(&global_ambus_cond);
  pthread_mutex_unlock(&global_ambus_mutex);
  ambus_run(ambus);
  return NULL;
}

struct aml_dbus *ambus_ensure_run(struct aml_dbus **ppambus) {
  static struct aml_dbus *ambus_invalid = (struct aml_dbus *)-1;
  struct aml_dbus *tmpbus = NULL;
  if (ppambus == NULL)
    ppambus = &tmpbus;
  if (*ppambus == NULL || *ppambus == ambus_invalid) {
    pthread_mutex_lock(&global_ambus_mutex);
    if (*ppambus == NULL) {
      pthread_t t;
      *ppambus = ambus_invalid;
      pthread_create(&t, NULL, ambus_dispatcher, ppambus);
    }
    while (*ppambus == ambus_invalid)
      pthread_cond_wait(&global_ambus_cond, &global_ambus_mutex);
    pthread_mutex_unlock(&global_ambus_mutex);
  }
  return *ppambus;
}

int ambus_run(struct aml_dbus *ambus) {
  ambus->dispatch_thread = pthread_self();
  return sd_event_loop(ambus->event);
}

sd_bus *ambus_sdbus(struct aml_dbus *ambus) { return ambus->bus; }

sd_event *ambus_sdevent(struct aml_dbus *ambus) { return ambus->event; }

bool ambus_in_dispatch_thread(struct aml_dbus *ambus) { return pthread_equal(pthread_self(), ambus->dispatch_thread); }

void ambus_wait_flag(struct aml_dbus *ambus, uint32_t *val, uint32_t mask) {
  pthread_mutex_lock(&ambus->lock);
  while ((*val & mask) == 0)
    pthread_cond_wait(&ambus->cond, &ambus->lock);
  pthread_mutex_unlock(&ambus->lock);
}

void ambus_set_flag(struct aml_dbus *ambus, uint32_t *val, uint32_t mask) {
  pthread_mutex_lock(&ambus->lock);
  *val |= mask;
  pthread_cond_broadcast(&ambus->cond);
  pthread_mutex_unlock(&ambus->lock);
}

int ambus_call_async(struct aml_dbus *ambus, sd_bus_message *m, sd_bus_message_handler_t callback, void *userdata) {
  int r = -1;
  RUN_IN_DISPATCH_THREAD(ambus, r = sd_bus_call_async(ambus->bus, NULL, m, callback, userdata, 0));
  return r;
}

sd_bus_message *ambus_call_sync(struct aml_dbus *ambus, sd_bus_message *m) {
  int r = -1;
  sd_bus_message *ret = NULL;
  if (ambus_in_dispatch_thread(ambus)) {
    AMBUS_LOGW("cannot make sync call in dispatch thread");
    return NULL;
  } else {
    int done = 0;
    int method_done(sd_bus_message * dm, void *userdata, sd_bus_error *ret_error) {
      ret = sd_bus_message_ref(dm);
      AMBUS_LOGD("call done, notify caller");
      ambus_set_flag(ambus, &done, 1);
      return 0;
    }
    POST_AND_WAIT_DISPATCH(ambus, r = sd_bus_call_async(ambus->bus, NULL, m, method_done, NULL, 0));
    if (r >= 0)
      ambus_wait_flag(ambus, &done, 1);
  }
  return ret;
}

int ambus_call_simple_async_va(struct aml_dbus *ambus, struct ambus_interface *intf, int idx,
                               sd_bus_message_handler_t callback, void *userdata, va_list ap) {
  int r = -1;
  int done = 0;
  void do_task(void *p) {
    sd_bus_message *msg = NULL;
    r = sd_bus_message_new_method_call(ambus->bus, &msg, intf->service, intf->object, intf->interface,
                                       intf->vtable[idx].member);
    if (r >= 0 && intf->vtable[idx].signature)
      r = sd_bus_message_appendv(msg, intf->vtable[idx].signature, ap);
    if (r >= 0)
      r = sd_bus_call_async(ambus->bus, NULL, msg, callback, userdata, 0);
    ambus_set_flag(ambus, &done, 1);
  }
  if (ambus_in_dispatch_thread(ambus))
    do_task(NULL);
  else if (ambus_post_task(ambus, do_task, NULL) == 0)
    ambus_wait_flag(ambus, &done, 1);
  return r;
}

int ambus_call_simple_async(struct aml_dbus *ambus, struct ambus_interface *intf, int idx,
                            sd_bus_message_handler_t callback, void *userdata, ...) {
  va_list ap;
  va_start(ap, userdata);
  int r = ambus_call_simple_async_va(ambus, intf, idx, callback, userdata, ap);
  va_end(ap);
  return r;
}

int ambus_call_simple_sync_va(struct aml_dbus *ambus, struct ambus_interface *intf, int idx, va_list ap) {
  if (ambus_in_dispatch_thread(ambus)) {
    AMBUS_LOGE("cannot make sync call in dispatch thread");
    return -1;
  }
  int r;
  int done = 0;
  int msg_handle(sd_bus_message * m, void *userdata, sd_bus_error *ret_error) {
    if (intf->vtable[idx].result)
      r = sd_bus_message_readv(m, intf->vtable[idx].result, ap);
    ambus_set_flag(ambus, &done, 1);
    return 0;
  }
  r = ambus_call_simple_async_va(ambus, intf, idx, msg_handle, NULL, ap);
  va_end(ap);
  if (r >= 0)
    ambus_wait_flag(ambus, &done, 1);
  return r;
}

int ambus_call_simple_sync(struct aml_dbus *ambus, struct ambus_interface *intf, int idx, ...) {
  va_list ap;
  va_start(ap, idx);
  int r = ambus_call_simple_sync_va(ambus, intf, idx, ap);
  va_end(ap);
  return r;
}

int ambus_call_sync_general(struct aml_dbus *ambus, const char *destination, const char *path, const char *interface,
                            const char *member, void *userdata, int (*msgpack)(sd_bus_message *m, void *userdata),
                            int (*msgunpack)(sd_bus_message *m, void *userdata)) {
  if (ambus_in_dispatch_thread(ambus)) {
    AMBUS_LOGE("cannot make sync call in dispatch thread");
    return -1;
  }
  int r = -1;
  int done = 0;
  int call_done(sd_bus_message * m, void *userdata, sd_bus_error *ret_error) {
    if (r >= 0 && msgunpack)
      r = msgunpack(m, userdata);
    ambus_set_flag(ambus, &done, 1);
    return 0;
  }
  void do_task(void *p) {
    sd_bus_message *msg = NULL;
    r = sd_bus_message_new_method_call(ambus->bus, &msg, destination, path, interface, member);
    if (r >= 0 && msgpack)
      r = msgpack(msg, userdata);
    if (r >= 0)
      r = sd_bus_call_async(ambus->bus, NULL, msg, call_done, userdata, 0);
    else {
      sd_bus_message_unref(msg);
      ambus_set_flag(ambus, &done, 1);
    }
  }
  if (ambus_post_task(ambus, do_task, NULL) == 0)
    ambus_wait_flag(ambus, &done, 1);
  return r;
}

int ambus_call_async_general(struct aml_dbus *ambus, const char *destination, const char *path, const char *interface,
                             const char *member, void *userdata, int (*msgpack)(sd_bus_message *m, void *userdata),
                             int (*reply_msg_handle)(sd_bus_message *m, void *userdata, sd_bus_error *ret_error)) {
  int r = -1;
  int done = 0;
  void do_task(void *p) {
    sd_bus_message *msg = NULL;
    r = sd_bus_message_new_method_call(ambus->bus, &msg, destination, path, interface, member);
    if (r >= 0 && msgpack)
      r = msgpack(msg, userdata);
    if (r >= 0)
      r = sd_bus_call_async(ambus->bus, NULL, msg, reply_msg_handle, userdata, 0);
    else {
      sd_bus_message_unref(msg);
    }
    if (p)
      ambus_set_flag(ambus, (uint32_t *)p, 1);
  }
  if (ambus_in_dispatch_thread(ambus)) {
    do_task(NULL);
  } else {
    uint32_t done = 0;
    if (ambus_post_task(ambus, do_task, &done) == 0)
      ambus_wait_flag(ambus, &done, 1);
  }
  return r;
}


int ambus_run_in_dispatch(struct aml_dbus *ambus, void (*cb)(void *), void *param) {
  if (ambus_in_dispatch_thread(ambus)) {
    cb(param);
  } else {
    uint32_t done = 0;
    void do_task(void *p) {
      cb(param);
      ambus_set_flag(ambus, &done, 1);
    }
    if (ambus_post_task(ambus, do_task, NULL) == 0)
      ambus_wait_flag(ambus, &done, 1);
  }
  return 0;
}
