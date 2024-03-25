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
#include <ctype.h>
#include <string>
#include <vector>

const char *test_service = "amlogic.yocto.test";
const char *test_object = "/amlogic/yocto/test/obj1";
const char *test_interface = "amlogic.yocto.test.intf1";

struct TestStruct {
  std::string name;
  int ival;
  float fval;
  bool bval;
};

int test_func1(int a1, const int a2, const int &a3, int &a4) {
  a4 = a1 + a2 + a3;
  int ret = a1;
  fprintf(stderr, "call test_func1(a1=%d,a2=%d, a3=%d, a4=%d, ret=%d\n", a1, a2, a3, a4, ret);
  return ret;
}

int test_func2(const std::string &arg1, std::vector<std::string> &out1, TestStruct & out2) {
  fprintf(stderr, "call test_func2 %s\n", arg1.c_str());
  out1 = {"1", "2", "3", arg1};
  out2 = {arg1, arg1.size(), arg1.size() * 1.0, isdigit(arg1[0])};
  return 0;
}

std::unordered_map<std::string, TestStruct> test_func3() {
  return {{"key1", {"n1", 10, 0.123, true}}, {"key2", {"n2", 20, 0.456, false}}};
}

AMBUS_STRUCT_DEFINE(TestStruct, name, ival, fval, bval);

static int ambus_method_handler(sd_bus_message *m, void *userdata, sd_bus_error *ret_error) {
  struct ambus_method_call<> *pcall = (struct ambus_method_call<> *)userdata;
  return pcall->call(m);
}

#define NEW_METHOD_ITEM(P, f, ...) new ambus_method_call<decltype(&f)>(#f, &f),
#define NEW_METHOD_VTABLE(P, f, ...)                                                                                   \
  SD_BUS_METHOD_WITH_OFFSET(#f, ambus_method_call<decltype(&f)>::data_pack::signature_input::value,                    \
                            ambus_method_call<decltype(&f)>::signature_output::value, ambus_method_handler,            \
                            (size_t)*P##_methods_ptr++, 0),

#define REGISTER_DBUS_METHOD(P, ...)                                                                                   \
  static const ambus_method_call<> *P##_methods[] = {MACRO_MAP(NEW_METHOD_ITEM, P, __VA_ARGS__)},                      \
                                   **P##_methods_ptr = P##_methods;                                                    \
  static const sd_bus_vtable P##_vtable[] = {SD_BUS_VTABLE_START(0),                                                   \
                                             MACRO_MAP(NEW_METHOD_VTABLE, P, __VA_ARGS__) SD_BUS_VTABLE_END};

#define CALL_DBUS(f, ...)                                                                                              \
  ambus_method_proxy<decltype(&f)>::call(ambus, test_service, test_object, test_interface, #f, ##__VA_ARGS__)

REGISTER_DBUS_METHOD(test, test_func1, test_func2, test_func3);
// above macro will generate code similar as the comment line below:
#if 0
static const ambus_method_call<> *test_methods[] = {
    new ambus_method_call<decltype(&test_func1)>("test_func1", &test_func1),
    new ambus_method_call<decltype(&test_func2)>("test_func2", &test_func2),
};

static const sd_bus_vtable test_vtable[] = {
    SD_BUS_VTABLE_START(0),
    SD_BUS_METHOD_WITH_OFFSET("test_func1", ambus_method_call<decltype(&test_func1)>::data_pack::signature_input::value,
                              ambus_method_call<decltype(&test_func1)>::signature_output::value, ambus_method_handler,
                              (size_t)test_methods[0], 0),
    SD_BUS_METHOD_WITH_OFFSET("test_func2", ambus_method_call<decltype(&test_func2)>::data_pack::signature_input::value,
                              ambus_method_call<decltype(&test_func2)>::signature_output::value, ambus_method_handler,
                              (size_t)test_methods[1], 0),
    SD_BUS_VTABLE_END};

#endif

int main(int argc, char *argv[]) {
  int r;
  if (argc != 2) {
    printf("USAGE: %s server/client\n", argv[0]);
    return 1;
  }
  if (strcmp(argv[1], "server") == 0) {
    struct aml_dbus *ambus = ambus_new(NULL, 0);
    sd_bus_add_object_vtable(ambus_sdbus(ambus), NULL, test_object, test_interface, test_vtable, NULL);
    sd_bus_request_name(ambus_sdbus(ambus), test_service, 0LL);
    ambus_run(ambus);
  } else if (strcmp(argv[1], "client") == 0) {
    struct aml_dbus *ambus = ambus_ensure_run(NULL);
    int a1 = 1;
    int a2 = 2;
    const int a3 = 10;
    int a4 = 0;
    // int r = ambus_method_proxy<decltype(&test_func1)>::call(ambus, test_service, test_object,
    // test_interface, "test_func1", a1, a2, a3, a4);
    int r = CALL_DBUS(test_func1, a1, a2, a3, a4);
    printf("call test_func1 %d %d %d %d ret %d\n", a1, a2, a3, a4, r);

    const std::string arg1 = "arg1";
    std::vector<std::string> out1;
    TestStruct out2;
    r = ambus_method_proxy<decltype(&test_func2)>::call(ambus, test_service, test_object, test_interface, "test_func2",
                                                        arg1, out1, out2);
    printf("call test_func2(%s) ret %d, out2: name:%s ival:%d fval:%f bval:%d out1:", arg1.c_str(), r,
           out2.name.c_str(), out2.ival, out2.fval, out2.bval);
    for (auto &e : out1)
      printf("%s ", e.c_str());
    printf("\n");

    const std::unordered_map<std::string, TestStruct> &out3 = CALL_DBUS(test_func3);
    printf("call test_func3 return ");
    for (auto &e : out3)
      printf("%s:(%s,%d,%f,%d),", e.first.c_str(), e.second.name.c_str(), e.second.ival, e.second.fval, e.second.bval);
    printf("\n");
  }
  return 0;
}
