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

#include "../include/property.h"

#include <algorithm>
#include <list>
#include <memory>
#include <mutex>

#include "defs.h"

#include "../common/log.h"
#define LOG_TAG "PropertyClient"

namespace Amlogic {
namespace Platform {
namespace Property {

#define CALL_DBUS(f, ...)                                                      \
  ambus_method_proxy<decltype(&f)>::call(                                      \
      ambus_ensure_run(&ambus_method), PROP_DBUS_SERVICE, PROP_DBUS_OBJECT,    \
      PROP_DBUS_INTERFACE, #f, ##__VA_ARGS__)

static struct aml_dbus *ambus_signal = nullptr;
static struct aml_dbus *ambus_method = nullptr;
static std::mutex m_lock;

struct CallbackInfo {
  PropertyChangedCallback cb;
  sd_bus_slot *match_slot;
  CallbackInfo(PropertyChangedCallback cb, sd_bus_slot *slot)
      : cb(cb), match_slot(slot) {}
};

struct CallbackInfoDeleter {
  void operator()(CallbackInfo *info) const {
    if (info->match_slot) {
      ambus_runnable_task([&]() {
        sd_bus_slot_unref(info->match_slot);
      }).run_on(ambus_ensure_run(&ambus_signal));
    }
    delete info;
  }
};

using CallbackInfoPtr = std::unique_ptr<CallbackInfo, CallbackInfoDeleter>;

static std::list<CallbackInfoPtr> m_callbacks;

bool get(const std::string &key, std::string &value) {
  std::string v;
  bool ret = CALL_DBUS(get, key, v);
  if (ret) {
    value = v;
  }
  return ret;
}

bool set(const std::string &key, const std::string &value) {
  return CALL_DBUS(set, key, value);
}

std::map<std::string, std::string> list() { return CALL_DBUS(list); }

static int dbus_signal_handle(sd_bus_message *message, void *userdata,
                              sd_bus_error *error) {
  PropertyChangedEvent e;

  if (ambus_data_pack<int, std::string, std::string, std::string, std::string,
                      std::string>::unpackall(false, message, e.event, e.ns,
                                              e.key, e.action, e.value,
                                              e.oldvalue) < 0) {
    LOGE("failed to unpack message\n");
    return 0;
  }
  PropertyChangedCallback callback = (PropertyChangedCallback)userdata;
  const std::lock_guard<std::mutex> lock(m_lock);
  if (std::find_if(m_callbacks.begin(), m_callbacks.end(),
                   [&](CallbackInfoPtr &info) {
                     return info->cb == callback;
                   }) == m_callbacks.end()) {
    LOGW("callback %p unsubscribed\n", callback);
    return 0;
  }
  // trigger callback
  callback(e);
  return 0;
}

bool subscribe(PropertyChangedCallback callback) {
  const std::lock_guard<std::mutex> lock(m_lock);
  struct aml_dbus *ambus = ambus_ensure_run(&ambus_signal);
  sd_bus_slot *m = nullptr;
  ambus_runnable_task([&]() {
    int ret = sd_bus_match_signal(ambus_sdbus(ambus), &m, PROP_DBUS_SERVICE,
                                  PROP_DBUS_OBJECT, PROP_DBUS_INTERFACE,
                                  PROP_DBUS_SIGNAL, dbus_signal_handle,
                                  (void *)callback);
  }).run_on(ambus);

  CallbackInfoPtr info(new CallbackInfo(callback, m));
  m_callbacks.push_back(std::move(info));

  return true;
}

void unsubscribe(PropertyChangedCallback callback) {
  const std::lock_guard<std::mutex> lock(m_lock);

  m_callbacks.erase(std::remove_if(m_callbacks.begin(), m_callbacks.end(),
                                   [&](const CallbackInfoPtr &info) {
                                     return info->cb == callback;
                                   }),
                    m_callbacks.end());
}

bool subscribe_one(PropertyChangedCallback callback, const std::string &key) {
  const std::lock_guard<std::mutex> lock(m_lock);

  struct aml_dbus *ambus = ambus_ensure_run(&ambus_signal);
  sd_bus_slot *m = nullptr;
  char match[512];
  snprintf(match, sizeof(match),
           "type='signal',sender='%s',path='%s',interface='%s',member='%s',"
           "arg2='%s'",
           PROP_DBUS_SERVICE, PROP_DBUS_OBJECT, PROP_DBUS_INTERFACE,
           PROP_DBUS_SIGNAL, key.c_str());
  ambus_runnable_task([&]() {
    int ret = sd_bus_add_match(ambus_sdbus(ambus), &m, match,
                               dbus_signal_handle, (void *)callback);
  }).run_on(ambus);

  CallbackInfoPtr info(new CallbackInfo(callback, m));
  m_callbacks.push_back(std::move(info));

  return true;
}

} // namespace Property
} // namespace Platform
} // namespace Amlogic
