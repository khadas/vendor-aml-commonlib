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

#include <map>
#include <string>

#include "../common/log.h"
#include "../property-db/property-db.h"

#include "defs.h"

#define LOG_TAG "PropertyServer"

using namespace Amlogic::Platform;

namespace internal {
bool get(const std::string &key, std::string &value) {
  return Property::get(key, value);
}

bool set(const std::string &key, const std::string &value) {
  return Property::set(key, value);
}

std::map<std::string, std::string> list() { return Property::list(); }
} // namespace internal

static const ambus_method_call<> *prop_methods[] = {
    new ambus_method_call<decltype(&internal::get)>("get", &internal::get),
    new ambus_method_call<decltype(&internal::set)>("set", &internal::set),
    new ambus_method_call<decltype(&internal::list)>("list", &internal::list),
};

static const sd_bus_vtable prop_vtable[] = {
    SD_BUS_VTABLE_START(0),
    SD_BUS_METHOD_WITH_OFFSET(
        "get",
        ambus_method_call<decltype(
            &internal::get)>::data_pack::signature_input::value,
        ambus_method_call<decltype(&internal::get)>::signature_output::value,
        ambus_method_call<>::handler, (size_t)prop_methods[0], SD_BUS_VTABLE_UNPRIVILEGED),
    SD_BUS_METHOD_WITH_OFFSET(
        "set",
        ambus_method_call<decltype(
            &internal::set)>::data_pack::signature_input::value,
        ambus_method_call<decltype(&internal::set)>::signature_output::value,
        ambus_method_call<>::handler, (size_t)prop_methods[1], SD_BUS_VTABLE_UNPRIVILEGED),
    SD_BUS_METHOD_WITH_OFFSET(
        "list",
        ambus_method_call<decltype(
            &internal::list)>::data_pack::signature_input::value,
        ambus_method_call<decltype(&internal::list)>::signature_output::value,
        ambus_method_call<>::handler, (size_t)prop_methods[2], SD_BUS_VTABLE_UNPRIVILEGED),
    SD_BUS_SIGNAL_WITH_NAMES(PROP_DBUS_SIGNAL, "isssss",
                             SD_BUS_PARAM(event) SD_BUS_PARAM(namespace)
                                 SD_BUS_PARAM(key) SD_BUS_PARAM(action)
                                     SD_BUS_PARAM(value) SD_BUS_PARAM(oldvalue),
                             0),
    SD_BUS_VTABLE_END};

static struct aml_dbus *ambus = nullptr;

static void onChanged(const Property::PropertyChangedEvent &e) {
  // emitChanged(e.key, e.value, e.action, e.oldvalue);
  ambus_runnable_task([&]() {
    sd_bus *bus = ambus_sdbus(ambus);
    sd_bus_message *msg = NULL;
    int r = sd_bus_message_new_signal(bus, &msg, PROP_DBUS_OBJECT,
                                      PROP_DBUS_INTERFACE, PROP_DBUS_SIGNAL);
    if (r >= 0) {
      r = ambus_data_pack<int, std::string, std::string, std::string,
                          std::string, std::string>::packall(false, msg,
                                                             e.event, e.ns,
                                                             e.key, e.action,
                                                             e.value,
                                                             e.oldvalue);
      if (r >= 0)
        r = sd_bus_send(bus, msg, NULL);
      if (r < 0) {
        LOGE("fail to send signal on %s %s %d\n", PROP_DBUS_OBJECT,
             PROP_DBUS_INTERFACE, r);
      }
    }
    if (msg)
      sd_bus_message_unref(msg);
  }).run_on(ambus);
}

int main() {
  Property::init();
  Property::subscribe(onChanged);

  ambus = ambus_new(nullptr, 0);
  sd_bus_add_object_vtable(ambus_sdbus(ambus), NULL, PROP_DBUS_OBJECT,
                           PROP_DBUS_INTERFACE, prop_vtable, NULL);
  sd_bus_request_name(ambus_sdbus(ambus), PROP_DBUS_SERVICE, 0LL);

  ambus_run(ambus);

  ambus_free(ambus);
  return 0;
}
