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
    else
      ambus_set_flag(ambus, &done, 1);
    if (msg)
      sd_bus_message_unref(msg);
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
    if (msg)
      sd_bus_message_unref(msg);
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

int ambus_dispatch_to_implement(sd_bus_message *m, struct ambus_data_pack_info *pi, int (*call)(void)) {
  int ret = ambus_unpack_all(m, pi);
  if (ret >= 0)
    ret = call();
  if (ret >= 0) {
    sd_bus_message *reply = NULL;
    ret = sd_bus_message_new_method_return(m, &reply);
    if (ret >= 0) {
      ret = ambus_pack_all(reply, pi);
      if (ret >= 0)
        ret = sd_bus_send(NULL, reply, NULL);
    }
    if (reply)
      sd_bus_message_unref(reply);
  }
  ambus_free_all(pi);
  return ret;
}

int ambus_call_sync_with_packer(struct aml_dbus *ambus, const char *destination, const char *path,
                                const char *interface, const char *member, struct ambus_data_pack_info *pi) {
  if (ambus_in_dispatch_thread(ambus)) {
    AMBUS_LOGE("cannot make sync call in dispatch thread");
    return -1;
  }
  int r = -1;
  int done = 0;
  int call_done(sd_bus_message * m, void *userdata, sd_bus_error *ret_error) {
    if (sd_bus_message_is_method_error(m, NULL)) {
      r = -sd_bus_message_get_errno(m);
      AMBUS_LOGW("call %s.%s fail %d %s\n", interface, member, r, sd_bus_message_get_error(m)->message);
    }
    if (r >= 0)
      r = ambus_unpack_all(m, pi);
    ambus_set_flag(ambus, &done, 1);
    return 0;
  }
  void do_task(void *p) {
    sd_bus_message *msg = NULL;
    r = sd_bus_message_new_method_call(ambus->bus, &msg, destination, path, interface, member);
    if (r >= 0)
      r = ambus_pack_all(msg, pi);
    if (r >= 0)
      r = sd_bus_call_async(ambus->bus, NULL, msg, call_done, NULL, 0);
    else
      ambus_set_flag(ambus, &done, 1);
    if (msg)
      sd_bus_message_unref(msg);
  }
  if (ambus_post_task(ambus, do_task, NULL) == 0)
    ambus_wait_flag(ambus, &done, 1);
  return r;
}

int ambus_data_pack_basic(struct ambus_data_type_info *t, sd_bus_message *m, void *val) {
  return sd_bus_message_append_basic(m, t->signature[0], val);
}

int ambus_data_unpack_basic(struct ambus_data_type_info *t, sd_bus_message *m, void *val) {
  return sd_bus_message_read_basic(m, t->signature[0], val);
}

int ambus_data_pack_string(struct ambus_data_type_info *t, sd_bus_message *m, void *val) {
  return sd_bus_message_append_basic(m, t->signature[0], *(const char **)val);
}

int ambus_data_unpack_string(struct ambus_data_type_info *t, sd_bus_message *m, void *val) {
  const char *s = NULL;
  int r = sd_bus_message_read_basic(m, t->signature[0], &s);
  if (r >= 0 && s)
    *(const char **)val = strdup(s);
  return r;
}

void ambus_data_free_string(struct ambus_data_type_info *t, void *val) { free(*(void **)val); }

int ambus_data_unpack_unixfd(struct ambus_data_type_info *t, sd_bus_message *m, void *val) {
  int fd = 0;
  int r = sd_bus_message_read_basic(m, t->signature[0], &fd);
  if (r >=0 && fd > 0)
    *(int *)val = dup(fd);
  return r;
}

void ambus_data_free_unixfd(struct ambus_data_type_info *t, void *val) {
  int fd = *(int *)val;
  if (fd > 0)
    close(fd);
}

AMBUS_DEFINE_DATA_TYPE(uint8_t, ambus_data_pack_basic, ambus_data_unpack_basic, NULL);
AMBUS_DEFINE_DATA_TYPE(bool, ambus_data_pack_basic, ambus_data_unpack_basic, NULL);
AMBUS_DEFINE_DATA_TYPE(int16_t, ambus_data_pack_basic, ambus_data_unpack_basic, NULL);
AMBUS_DEFINE_DATA_TYPE(uint16_t, ambus_data_pack_basic, ambus_data_unpack_basic, NULL);
AMBUS_DEFINE_DATA_TYPE(int32_t, ambus_data_pack_basic, ambus_data_unpack_basic, NULL);
AMBUS_DEFINE_DATA_TYPE(uint32_t, ambus_data_pack_basic, ambus_data_unpack_basic, NULL);
AMBUS_DEFINE_DATA_TYPE(int64_t, ambus_data_pack_basic, ambus_data_unpack_basic, NULL);
AMBUS_DEFINE_DATA_TYPE(uint64_t, ambus_data_pack_basic, ambus_data_unpack_basic, NULL);
AMBUS_DEFINE_DATA_TYPE(double, ambus_data_pack_basic, ambus_data_unpack_basic, NULL);
AMBUS_DEFINE_DATA_TYPE(string, ambus_data_pack_string, ambus_data_unpack_string, ambus_data_free_string);
AMBUS_DEFINE_DATA_TYPE(unixfd, ambus_data_pack_basic, ambus_data_unpack_unixfd, ambus_data_free_unixfd);

int ambus_pack_all(sd_bus_message *m, struct ambus_data_pack_info *pi) {
  int r = 0;
  for (; r >= 0 && pi->type; pi++) {
    if (pi->output == 0)
      r = pi->type->pack(pi->type, m, pi->val);
  }
  return r;
}

int ambus_unpack_all(sd_bus_message *m, struct ambus_data_pack_info *pi) {
  int r = 0;
  for (; r >= 0 && pi->type; pi++) {
    if (pi->output == 1)
      r = pi->type->unpack(pi->type, m, pi->val);
  }
  return r;
}

void ambus_free_all(struct ambus_data_pack_info *pi) {
  for (; pi->type; pi++) {
    if (pi->type->free)
      pi->type->free(pi->type, pi->val);
  }
}

struct ambus_data_list_node {
  struct ambus_data_list_node *next;
};

int ambus_data_pack_array(struct ambus_data_type_info *t, sd_bus_message *m, void *val) {
  struct ambus_data_type_info_array *ta = (struct ambus_data_type_info_array *)t;
  struct ambus_data_type_info *pi = ta->dinfo;
  void *data = *(void **)val;
  void *ptr = data + ta->offset;
  int r = sd_bus_message_open_container(m, t->signature[0], t->signature + 1);
  for (int i = *(int *)data; r >= 0 && i--;) {
    r = pi->pack(pi, m, ptr);
    ptr += ta->size;
  }
  sd_bus_message_close_container(m);
  return r;
}

int ambus_data_unpack_array(struct ambus_data_type_info *t, sd_bus_message *m, void *val) {
  struct ambus_data_type_info_array *ta = (struct ambus_data_type_info_array *)t;
  struct ambus_data_type_info *pi = ta->dinfo;
  int r = sd_bus_message_enter_container(m, t->signature[0], t->signature + 1);
  struct ambus_data_list_node *head = alloca(ta->ptr_offset + ta->size), *node = head;
  int num;
  for (num = 0; (r = pi->unpack(pi, m, (void *)node + ta->ptr_offset)) > 0; num++) {
    node->next = alloca(ta->ptr_offset + ta->size);
    node = node->next;
  }
  sd_bus_message_exit_container(m);
  if (r >= 0) {
    void *data = *(void **)val = malloc(ta->offset + ta->size * num);
    *(int *)data = num;
    void *ptr = data + ta->offset;
    for (; num--; head = head->next, ptr += ta->size) {
      memcpy(ptr, (void *)head + ta->ptr_offset, ta->size);
    }
  }
  return r;
}

void ambus_data_free_array(struct ambus_data_type_info *t, void *val) {
  struct ambus_data_type_info_array *ta = (struct ambus_data_type_info_array *)t;
  struct ambus_data_type_info *pi = ta->dinfo;
  void *data = *(void **)val;
  if (data == NULL)
    return;
  void *ptr = data + ta->offset;
  if (pi->free) {
    for (int i = *(int *)data; i--;) {
      pi->free(pi, ptr);
      ptr += ta->size;
    }
  }
  free(data);
}

int ambus_data_pack_struct(struct ambus_data_type_info *t, sd_bus_message *m, void *val) {
  struct ambus_data_type_info_struct *ts = (struct ambus_data_type_info_struct *)t;
  int r = sd_bus_message_open_container(m, 'r', ts->contents);
  void *ptr = *(void **)val;
  for (typeof(ts->fields_info[0]) *f = ts->fields_info; r >= 0 && f->finfo; f++) {
    r = f->finfo->pack(f->finfo, m, ptr + f->offset);
  }
  sd_bus_message_close_container(m);
  return r;
}

int ambus_data_unpack_struct(struct ambus_data_type_info *t, sd_bus_message *m, void *val) {
  struct ambus_data_type_info_struct *ts = (struct ambus_data_type_info_struct *)t;
  int r = sd_bus_message_enter_container(m, 'r', ts->contents);
  void *ptr = *(void **)val = malloc(ts->size);
  for (typeof(ts->fields_info[0]) *f = ts->fields_info; r >= 0 && f->finfo; f++) {
    r = f->finfo->unpack(f->finfo, m, ptr + f->offset);
  }
  sd_bus_message_exit_container(m);
  return r;
}

void ambus_data_free_struct(struct ambus_data_type_info *t, void *val) {
  struct ambus_data_type_info_struct *ts = (struct ambus_data_type_info_struct *)t;
  void *ptr = *(void **)val;
  if (ptr == NULL)
    return;
  for (typeof(ts->fields_info[0]) *f = ts->fields_info; f->finfo; f++) {
    if (f->finfo->free)
      f->finfo->free(f->finfo, ptr + f->offset);
  }
  free(ptr);
}

int ambus_data_pack_map(struct ambus_data_type_info *t, sd_bus_message *m, void *val) {
  struct ambus_data_type_info_map *ts = (struct ambus_data_type_info_map *)t;
  struct ambus_data_list_node *ptr = *(struct ambus_data_list_node **)val;
  int r;
  AMBUS_CHECK(r, sd_bus_message_open_container(m, 'a', t->signature + 1), return r);
  for (; r >= 0 && ptr; ptr = ptr->next) {
    AMBUS_CHECK(r, sd_bus_message_open_container(m, 'e', ts->contents), break);
    AMBUS_CHECK(r, ts->kinfo->pack(ts->kinfo, m, (void *)ptr + ts->koff), break);
    AMBUS_CHECK(r, ts->vinfo->pack(ts->vinfo, m, (void *)ptr + ts->voff), break);
    sd_bus_message_close_container(m);
  }
  sd_bus_message_close_container(m);
  return r;
}

int ambus_data_unpack_map(struct ambus_data_type_info *t, sd_bus_message *m, void *val) {
  struct ambus_data_type_info_map *ts = (struct ambus_data_type_info_map *)t;
  struct ambus_data_list_node **tail = (struct ambus_data_list_node **)val;
  int r = sd_bus_message_enter_container(m, 'a', t->signature + 1);
  while (r > 0 && (r = sd_bus_message_enter_container(m, 'e', ts->contents)) > 0) {
    struct ambus_data_list_node *cur = malloc(ts->size);
    if ((r = ts->kinfo->unpack(ts->kinfo, m, (void *)cur + ts->koff)) > 0 &&
        (r = ts->vinfo->unpack(ts->vinfo, m, (void *)cur + ts->voff)) > 0) {
      *tail = cur;
      tail = &cur->next;
    } else {
      free(cur);
    }
    sd_bus_message_exit_container(m);
  }
  *tail = NULL;
  return r;
}

void ambus_data_free_map(struct ambus_data_type_info *t, void *val) {
  struct ambus_data_type_info_map *ts = (struct ambus_data_type_info_map *)t;
  for (struct ambus_data_list_node *ptr = *(struct ambus_data_list_node **)val; ptr;) {
    struct ambus_data_list_node *next = ptr->next;
    if (ts->kinfo->free)
      ts->kinfo->free(ts->kinfo, (void *)ptr + ts->koff);
    if (ts->vinfo->free)
      ts->vinfo->free(ts->vinfo, (void *)ptr + ts->voff);
    free(ptr);
    ptr = next;
  }
}

