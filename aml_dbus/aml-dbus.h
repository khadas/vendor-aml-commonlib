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

#include <pthread.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <systemd/sd-bus.h>
#include <systemd/sd-event.h>

#ifndef AML_DBUS_H
#define AML_DBUS_H

#define AMBUS_LOGE(fmt, ...) fprintf(stderr, "%s:%d " fmt "\n", __func__, __LINE__, ##__VA_ARGS__)
#define AMBUS_LOGW(fmt, ...) fprintf(stderr, "%s:%d " fmt "\n", __func__, __LINE__, ##__VA_ARGS__)
#define AMBUS_LOGI(fmt, ...) fprintf(stderr, "%s:%d " fmt "\n", __func__, __LINE__, ##__VA_ARGS__)
#define AMBUS_LOGD(fmt, ...) fprintf(stderr, "%s:%d " fmt "\n", __func__, __LINE__, ##__VA_ARGS__)
#define AMBUS_LOGV(fmt, ...) fprintf(stderr, "%s:%d " fmt "\n", __func__, __LINE__, ##__VA_ARGS__)

#define MACRO_CAT(a, b) a##b
#define MACRO_CAT2(a, b) MACRO_CAT(a, b)
#define MACRO_STRINGIFY(a) #a
#define MACRO_STRINGIFY2(a) MACRO_STRINGIFY(a)
#define MACRO_GET_8TH(_1,_2,_3,_4,_5,_6,_7,_8,...) _8
#define MACRO_GET1ST(...) MACRO_GET_8TH(_1,_2,_3,_4,_5,_6,_7,__VA_ARGS__)
#define MACRO_GET2ND(...) MACRO_GET_8TH(_1,_2,_3,_4,_5,_6,__VA_ARGS__)
#define MACRO_GET3RD(...) MACRO_GET_8TH(_1,_2,_3,_4,_5,__VA_ARGS__)
#define MACRO_GET4TH(...) MACRO_GET_8TH(_1,_2,_3,_4,__VA_ARGS__)
#define MACRO_GET5TH(...) MACRO_GET_8TH(_1,_2,_3,__VA_ARGS__)
#define MACRO_GET6TH(...) MACRO_GET_8TH(_1,_2,__VA_ARGS__)
#define MACRO_GET7TH(...) MACRO_GET_8TH(_1,__VA_ARGS__)
#define MACRO_GET8TH(...) MACRO_GET_8TH(__VA_ARGS__)
#define MACRO_REMOVE_1ST_(a,...) __VA_ARGS__
#define MACRO_REMOVE_1ST(...) MACRO_REMOVE_1ST_(__VA_ARGS__)
#define MACRO_EMPTY(...)


// https://github.com/swansontec/map-macro
// modified to avoid name conflicts and add additional parameter
#define MACRO_EVAL0(...) __VA_ARGS__
#define MACRO_EVAL1(...) MACRO_EVAL0(MACRO_EVAL0(MACRO_EVAL0(__VA_ARGS__)))
#define MACRO_EVAL2(...) MACRO_EVAL1(MACRO_EVAL1(MACRO_EVAL1(__VA_ARGS__)))
#define MACRO_EVAL3(...) MACRO_EVAL2(MACRO_EVAL2(MACRO_EVAL2(__VA_ARGS__)))
#define MACRO_EVAL4(...) MACRO_EVAL3(MACRO_EVAL3(MACRO_EVAL3(__VA_ARGS__)))
#define MACRO_EVAL(...) MACRO_EVAL4(MACRO_EVAL4(MACRO_EVAL4(__VA_ARGS__)))

#define MACRO_MAP_END(...)
#define MACRO_MAP_OUT
#define MACRO_MAP_COMMA ,

#define MACRO_MAP_GET_END2() 0, MACRO_MAP_END
#define MACRO_MAP_GET_END1(...) MACRO_MAP_GET_END2
#define MACRO_MAP_GET_END(...) MACRO_MAP_GET_END1
#define MACRO_MAP_NEXT0(test, next, ...) next MACRO_MAP_OUT
#define MACRO_MAP_NEXT1(test, next) MACRO_MAP_NEXT0(test, next, 0)
#define MACRO_MAP_NEXT(test, next) MACRO_MAP_NEXT1(MACRO_MAP_GET_END test, next)

#define MACRO_MAP0(f, P, x, peek, ...) f(P, x) MACRO_MAP_NEXT(peek, MACRO_MAP1)(f, P, peek, __VA_ARGS__)
#define MACRO_MAP1(f, P, x, peek, ...) f(P, x) MACRO_MAP_NEXT(peek, MACRO_MAP0)(f, P, peek, __VA_ARGS__)
#define MACRO_MAP(f, P, ...) MACRO_EVAL(MACRO_MAP1(f, P, __VA_ARGS__, ()()(), ()()(), ()()(), 0))

#define MACRO_MAP_PAIR0(f, P, x, y, peek, ...) MACRO_MAP_NEXT(x,f)(P, x, y) MACRO_MAP_NEXT(peek, MACRO_MAP_PAIR1)(f, P, peek, __VA_ARGS__)
#define MACRO_MAP_PAIR1(f, P, x, y, peek, ...) MACRO_MAP_NEXT(x,f)(P, x, y) MACRO_MAP_NEXT(peek, MACRO_MAP_PAIR0)(f, P, peek, __VA_ARGS__)
#define MACRO_MAP_PAIR(f, P, ...) MACRO_EVAL(MACRO_MAP_PAIR1(f, P, ##__VA_ARGS__, ()()(), ()()(), ()()()))

#define AMBUS_CHECK(v, x, e)                                                                                           \
  do {                                                                                                                 \
    if ((v = (x)) < 0) {                                                                                               \
      AMBUS_LOGW("call %s fail %d %s\n", #x, v, strerror(-v));                                                         \
      e;                                                                                                               \
    }                                                                                                                  \
  } while (0)

#define POST_AND_WAIT_DISPATCH(ambus, thread_expr)                                                                     \
  do {                                                                                                                 \
    uint32_t done = 0;                                                                                                 \
    void do_task(void *p) {                                                                                            \
      thread_expr;                                                                                                     \
      ambus_set_flag(ambus, &done, 1);                                                                                 \
    }                                                                                                                  \
    if (ambus_post_task(ambus, do_task, NULL) == 0)                                                                    \
      ambus_wait_flag(ambus, &done, 1);                                                                                \
  } while (0)

#define RUN_IN_DISPATCH_THREAD(ambus, thread_expr)                                                                     \
  do {                                                                                                                 \
    if (!ambus_in_dispatch_thread(ambus)) {                                                                            \
      POST_AND_WAIT_DISPATCH(ambus, thread_expr);                                                                      \
    } else {                                                                                                           \
      thread_expr;                                                                                                     \
    }                                                                                                                  \
  } while (0)

#define AMBUS_INTERFACE_PTR(_intf) (&_intf##_interface)
#define AMBUS_VTABLE(_intf, _member) (AMBUS_INTERFACE_PTR(_intf)->vtable[AMBUS_MEMBER_IDX(_intf, _member)])
#define AMBUS_SERVICE(_intf) AMBUS_INTERFACE_PTR(_intf)->service
#define AMBUS_OBJECT(_intf) AMBUS_INTERFACE_PTR(_intf)->object
#define AMBUS_INTERFACE(_intf) AMBUS_INTERFACE_PTR(_intf)->interface
#define AMBUS_SERV_OBJ_INTF(_intf) AMBUS_SERVICE(_intf), AMBUS_OBJECT(_intf), AMBUS_INTERFACE(_intf)

#define AMBUS_MEMBER_IDX(_intf, _member) _intf##_enum_##_member
#define AMBUS_ADD_VTABLE(_ambus, _intf, _userData)                                                                     \
  sd_bus_add_object_vtable(ambus_sdbus(_ambus), NULL, AMBUS_OBJECT(_intf), AMBUS_INTERFACE(_intf), _intf##_vtable,     \
                           _userData)
#define AMBUS_NEW_SIGNAL(_sig, _ambus, _intf, _member)                                                                 \
  sd_bus_message_new_signal(ambus_sdbus(_ambus), _sig, AMBUS_OBJECT(_intf), AMBUS_INTERFACE(_intf), _member)
#define AMBUS_REQUEST_NAME(_ambus, _intf) sd_bus_request_name(ambus_sdbus(_ambus), AMBUS_SERVICE(_intf), 0)

#define GEN_ENUM_ITEM(p, _member, ...) AMBUS_MEMBER_IDX(p, _member),
#define GEN_EMPTY(...)
#define GEN_VTABLE2(p, _member, _signature) {#_member, _signature, NULL},
#define GEN_VTABLE3(p, _member, _signature, _result) {#_member, _signature, _result},
#define GEN_PROP_DECL(p, _member, _signature)                                                                          \
  static int p##_property_get(sd_bus *bus, const char *path, const char *interface, const char *property,              \
                              sd_bus_message *reply, void *userdata, sd_bus_error *ret_error);
#define GEN_PROPRW_DECL(p, _member, _signature)                                                                        \
  static int p##_property_get(sd_bus *bus, const char *path, const char *interface, const char *property,              \
                              sd_bus_message *reply, void *userdata, sd_bus_error *ret_error);                         \
  static int p##_property_set(sd_bus *bus, const char *path, const char *interface, const char *property,              \
                              sd_bus_message *value, void *userdata, sd_bus_error *ret_error);
#define GEN_METHOD_DECL(p, _member, _signature, _result)                                                               \
  static int p##_method_##_member(sd_bus_message *m, void *userdata, sd_bus_error *ret_error);

#define GEN_VTABLE_PROP(p, _member, _signature) SD_BUS_PROPERTY(#_member, _signature, p##_property_get, 0, 0),
#define GEN_VTABLE_PROPRW(p, _member, _signature)                                                                      \
  SD_BUS_WRITABLE_PROPERTY(#_member, _signature, p##_property_get, p##_property_set, 0, 0),
#define GEN_VTABLE_METHOD(p, _member, _signature, _result)                                                             \
  SD_BUS_METHOD(#_member, _signature, _result, p##_method_##_member, SD_BUS_VTABLE_UNPRIVILEGED),
#define GEN_VTABLE_SIGNAL(p, _member, _signature) SD_BUS_SIGNAL(#_member, _signature, 0),

#define AMBUS_DECLARE_INTERFACE(M)                                                                                     \
  enum { M(M, GEN_ENUM_ITEM, GEN_ENUM_ITEM, GEN_ENUM_ITEM, GEN_ENUM_ITEM) M##_enum_END_ };                             \
  extern struct ambus_interface M##_interface;

#define AMBUS_DEFINE_INTERFACE(M, service, object, interface)                                                          \
  struct ambus_interface M##_interface = {                                                                             \
      service, object, interface, {M(M, GEN_VTABLE2, GEN_VTABLE2, GEN_VTABLE3, GEN_VTABLE2){0, 0, 0}}};

#define AMBUS_DECLARE_VTABLE(M) M(M, GEN_PROP_DECL, GEN_PROPRW_DECL, GEN_METHOD_DECL, GEN_EMPTY)
#define AMBUS_DEFINE_VTABLE(M)                                                                                         \
  static const sd_bus_vtable M##_vtable[] = {                                                                          \
      SD_BUS_VTABLE_START(0),                                                                                          \
      M(M, GEN_VTABLE_PROP, GEN_VTABLE_PROPRW, GEN_VTABLE_METHOD, GEN_VTABLE_SIGNAL) SD_BUS_VTABLE_END};


#define AMBUS_DATA_TYPE_uint8_t "y", uint8_t
#define AMBUS_DATA_TYPE_bool "b", int
#define AMBUS_DATA_TYPE__Bool AMBUS_DATA_TYPE_bool
#define AMBUS_DATA_TYPE_int16_t "n", int16_t
#define AMBUS_DATA_TYPE_uint16_t "q", uint16_t
#define AMBUS_DATA_TYPE_int32_t "i", int32_t
#define AMBUS_DATA_TYPE_uint32_t "u", uint32_t
#define AMBUS_DATA_TYPE_int64_t "x", int64_t
#define AMBUS_DATA_TYPE_uint64_t "t", uint64_t
#define AMBUS_DATA_TYPE_double "d", double
#define AMBUS_DATA_TYPE_string "s", char *
#define AMBUS_DATA_TYPE_unixfd "h", int
// for a new type ttt, we need to provide macro AMBUS_DATA_TYPE_ttt and ambus_data_type_info_ttt

#define AMBUS_DATA_TYPE_SIG(x) MACRO_GET1ST(MACRO_CAT2(AMBUS_DATA_TYPE_,x))
#define AMBUS_DATA_TYPE_CT(x) MACRO_GET2ND(MACRO_CAT2(AMBUS_DATA_TYPE_, x), struct x *)

#define AMBUS_DATA_TYPE_INFO(x) MACRO_CAT2(ambus_data_type_info_, x)
#define AMBUS_DATA_TYPE_INFO_PTR(x) (struct ambus_data_type_info *)&AMBUS_DATA_TYPE_INFO(x)

#define AMBUS_DATA_TYPE_IS_OUT_OUT(x) _, x
#define AMBUS_DATA_TYPE_IS_OUT(x, t, f) MACRO_GET3RD(MACRO_CAT2(AMBUS_DATA_TYPE_IS_OUT_, x), t, f)
#define AMBUS_DATA_TYPE_STRIP(x) MACRO_GET2ND(MACRO_CAT2(AMBUS_DATA_TYPE_IS_OUT_, x), x)

#define GEN_API_DECLARE_1(P, x, y) , AMBUS_DATA_TYPE_CT(AMBUS_DATA_TYPE_STRIP(x)) AMBUS_DATA_TYPE_IS_OUT(x, *y, y)
#define GEN_API_DECLARE(P, f, ...) int f(MACRO_REMOVE_1ST(MACRO_MAP_PAIR(GEN_API_DECLARE_1, P, ##__VA_ARGS__)))

#define GEN_API_METHOD_IMPL_1(P, x, y) AMBUS_DATA_TYPE_CT(AMBUS_DATA_TYPE_STRIP(x)) y = 0;
#define GEN_API_METHOD_IMPL_2(P, x, y)                                                                                 \
  {AMBUS_DATA_TYPE_INFO_PTR(AMBUS_DATA_TYPE_STRIP(x)), &y, AMBUS_DATA_TYPE_IS_OUT(x, 0, 1)},
#define GEN_API_METHOD_IMPL_3(P, x, y) , AMBUS_DATA_TYPE_IS_OUT(x, &y, y)
// genrate dbus method handle f stub to C function impl
#define GEN_API_METHOD_IMPL_NAME(P, f, impl, ...)                                                                      \
  static int f(sd_bus_message *m, void *userdata, sd_bus_error *ret_error) {                                           \
    MACRO_MAP_PAIR(GEN_API_METHOD_IMPL_1, P, ##__VA_ARGS__)                                                            \
    struct ambus_data_pack_info dpinfo[] = {MACRO_MAP_PAIR(GEN_API_METHOD_IMPL_2, P, ##__VA_ARGS__){NULL, NULL, 0}};   \
    int call(void) { return impl(MACRO_REMOVE_1ST(MACRO_MAP_PAIR(GEN_API_METHOD_IMPL_3, P, ##__VA_ARGS__))); };        \
    return ambus_dispatch_to_implement(m, dpinfo, call);                                                               \
  }
#define GEN_API_METHOD_IMPL(P, f, ...) GEN_API_METHOD_IMPL_NAME(P, MACRO_CAT2(P, _method_##f), f, ##__VA_ARGS__)

#define GEN_API_METHOD_PROXY_2(P, x, y)                                                                                \
  {AMBUS_DATA_TYPE_INFO_PTR(AMBUS_DATA_TYPE_STRIP(x)), AMBUS_DATA_TYPE_IS_OUT(x, y, &y),                               \
   AMBUS_DATA_TYPE_IS_OUT(x, 1, 0)},
#define GEN_API_METHOD_PROXY_1(P, x, y) , AMBUS_DATA_TYPE_CT(AMBUS_DATA_TYPE_STRIP(x)) AMBUS_DATA_TYPE_IS_OUT(x, *y, y)
// generate function f, proxy to dbus method m
#define GEN_API_METHOD_PROXY_NAME(P, f, m, ...)                                                                        \
  int f(MACRO_REMOVE_1ST(MACRO_MAP_PAIR(GEN_API_METHOD_PROXY_1, P, ##__VA_ARGS__))) {                                  \
    struct ambus_data_pack_info dpinfo[] = {MACRO_MAP_PAIR(GEN_API_METHOD_PROXY_2, P, ##__VA_ARGS__){NULL, NULL, 0}};  \
    return ambus_call_sync_with_packer(P(), #m, dpinfo);                                                               \
  };
#define GEN_API_METHOD_PROXY(P, f, ...) GEN_API_METHOD_PROXY_NAME(P, f, f, ##__VA_ARGS__)

#define GEN_VTABLE_EX_5(P, x, y) AMBUS_DATA_TYPE_IS_OUT(x, AMBUS_DATA_TYPE_SIG(AMBUS_DATA_TYPE_STRIP(x)), "")
#define GEN_VTABLE_EX_4(P, x, y) AMBUS_DATA_TYPE_IS_OUT(x, "", AMBUS_DATA_TYPE_SIG(AMBUS_DATA_TYPE_STRIP(x)))
#define GEN_VTABLE_EX_3(P, f, ...)                                                                                     \
  {#f, MACRO_MAP_PAIR(GEN_VTABLE_EX_4, P, ##__VA_ARGS__), MACRO_MAP_PAIR(GEN_VTABLE_EX_5, P, ##__VA_ARGS__)},
#define GEN_VTABLE_EX_2(P,x,y) AMBUS_DATA_TYPE_SIG(AMBUS_DATA_TYPE_STRIP(x))
#define GEN_VTABLE_EX_1(P,f,...) {#f, MACRO_MAP_PAIR(GEN_VTABLE_EX_2, P, ##__VA_ARGS__), NULL},
#define AMBUS_DEFINE_INTERFACE_EX(M, service, object, interface)                                                       \
  struct ambus_interface M##_interface = {                                                                             \
      service,                                                                                                         \
      object,                                                                                                          \
      interface,                                                                                                       \
      {M(M, GEN_VTABLE_EX_1, GEN_VTABLE_EX_1, GEN_VTABLE_EX_3, GEN_VTABLE_EX_1){0, 0, 0}}};

#define GEN_VTABLE_PROP_EX(P, f, x, y) SD_BUS_PROPERTY(#f, AMBUS_DATA_TYPE_SIG(x), P##_property_get, 0, 0),
#define GEN_VTABLE_PROPRW_EX(P, f, x, y)                                                                               \
  SD_BUS_WRITABLE_PROPERTY(#f, AMBUS_DATA_TYPE_SIG(x), P##_property_get, P##_property_set, 0, 0),
#define GEN_VTABLE_METHOD_EX_1(P, x, y) AMBUS_DATA_TYPE_IS_OUT(x, "" , SD_BUS_PARAM(y))
#define GEN_VTABLE_METHOD_EX_2(P, x, y) AMBUS_DATA_TYPE_IS_OUT(x, SD_BUS_PARAM(y), "")
#define GEN_VTABLE_METHOD_EX(P, f, ...)                                                                                \
  SD_BUS_METHOD_WITH_NAMES(                                                                                            \
      #f, MACRO_MAP_PAIR(GEN_VTABLE_EX_4, P, ##__VA_ARGS__), MACRO_MAP_PAIR(GEN_VTABLE_METHOD_EX_1, P, ##__VA_ARGS__), \
      MACRO_MAP_PAIR(GEN_VTABLE_EX_5, P, ##__VA_ARGS__), MACRO_MAP_PAIR(GEN_VTABLE_METHOD_EX_2, P, ##__VA_ARGS__),     \
      P##_method_##f, SD_BUS_VTABLE_UNPRIVILEGED),
#define GEN_VTABLE_SIGNAL_EX(P, f, ...)                                                                                \
  SD_BUS_SIGNAL_WITH_NAMES(#f, MACRO_MAP_PAIR(GEN_VTABLE_EX_4, P, ##__VA_ARGS__),                                      \
                           MACRO_MAP_PAIR(GEN_VTABLE_METHOD_EX_1, P, ##__VA_ARGS__), 0),
#define AMBUS_DEFINE_VTABLE_ADDITIONAL(M, ADDITIONAL)                                                                  \
  M(M, GEN_EMPTY, GEN_EMPTY, GEN_API_METHOD_IMPL, GEN_EMPTY);                                                          \
  static const sd_bus_vtable M##_vtable[] = {SD_BUS_VTABLE_START(0),                                                   \
                                             M(M, GEN_VTABLE_PROP_EX, GEN_VTABLE_PROPRW_EX, GEN_VTABLE_METHOD_EX,      \
                                               GEN_VTABLE_SIGNAL_EX) ADDITIONAL() SD_BUS_VTABLE_END};
#define AMBUS_DEFINE_VTABLE_EX(M) AMBUS_DEFINE_VTABLE_ADDITIONAL(M, MACRO_EMPTY)

#define AMBUS_DEFINE_PROXY_EX(M,P) M(P, GEN_EMPTY, GEN_EMPTY, GEN_API_METHOD_PROXY, GEN_EMPTY)

#define AMBUS_DECLARE_DATA_TYPE(x, ...) extern struct ambus_data_type_info AMBUS_DATA_TYPE_INFO(x);
#define AMBUS_DEFINE_DATA_TYPE(x, ...)                                                                                 \
  struct ambus_data_type_info AMBUS_DATA_TYPE_INFO(x) = {AMBUS_DATA_TYPE_SIG(x), __VA_ARGS__};

#define AMBUS_DATA_TYPE_DECLARE_ARRAY(t, x)                                                                            \
  extern struct ambus_data_type_info_array AMBUS_DATA_TYPE_INFO(t);                                                    \
  struct t {                                                                                                           \
    int num;                                                                                                           \
    AMBUS_DATA_TYPE_CT(x) val[];                                                                                       \
  };

#define AMBUS_DATA_TYPE_DEFINE_ARRAY(t, x)                                                                             \
  struct ambus_data_type_info_array AMBUS_DATA_TYPE_INFO(t) = {                                                        \
      {AMBUS_DATA_TYPE_SIG(t), ambus_data_pack_array, ambus_data_unpack_array, ambus_data_free_array},                 \
      AMBUS_DATA_TYPE_INFO_PTR(x),                                                                                     \
      sizeof(AMBUS_DATA_TYPE_CT(x)),                                                                                   \
      offsetof(struct t, val),                                                                                         \
      offsetof(                                                                                                        \
          struct {                                                                                                     \
            void *next;                                                                                                \
            AMBUS_DATA_TYPE_CT(x) val[];                                                                               \
          },                                                                                                           \
          val)};

#define AMBUS_ARRAY_NEW(t, n)                                                                                          \
  ({                                                                                                                   \
    struct t *v = malloc(offsetof(struct t, val) + sizeof(v->val[0]) * n);                                             \
    v->num = n;                                                                                                        \
    v;                                                                                                                 \
  })

#define AMBUS_DATA_FREE(t, v) (AMBUS_DATA_TYPE_INFO_PTR(t))->free(AMBUS_DATA_TYPE_INFO_PTR(t), v)

#define GEN_STRUCT_DECLARE_1(P, x, y) AMBUS_DATA_TYPE_CT(x) y;
#define AMBUS_DATA_TYPE_DECLARE_STRUCT(t, ...)                                                                         \
  extern struct ambus_data_type_info_struct AMBUS_DATA_TYPE_INFO(t);                                                   \
  struct t {                                                                                                           \
    MACRO_MAP_PAIR(GEN_STRUCT_DECLARE_1, P, ##__VA_ARGS__)                                                             \
  };

#define GEN_STRUCT_DEFINE_1(P, x, y) {AMBUS_DATA_TYPE_INFO_PTR(x), offsetof(P, y)},
#define GEN_STRUCT_DEFINE_2(P, x, y) AMBUS_DATA_TYPE_SIG(x)
#define AMBUS_DATA_TYPE_DEFINE_STRUCT(t, ...)                                                                          \
  struct ambus_data_type_info_struct AMBUS_DATA_TYPE_INFO(t) = {                                                       \
      {AMBUS_DATA_TYPE_SIG(t), ambus_data_pack_struct, ambus_data_unpack_struct, ambus_data_free_struct},              \
      MACRO_MAP_PAIR(GEN_STRUCT_DEFINE_2, P, ##__VA_ARGS__),                                                           \
      sizeof(struct t),                                                                                                \
      {MACRO_MAP_PAIR(GEN_STRUCT_DEFINE_1, struct t, ##__VA_ARGS__) NULL}};

#define AMBUS_DATA_TYPE_DECLARE_MAP(t, KEY, key, VAL, val)                                                             \
  extern struct ambus_data_type_info_map AMBUS_DATA_TYPE_INFO(t);                                                      \
  struct t {                                                                                                           \
    struct t *next;                                                                                                    \
    AMBUS_DATA_TYPE_CT(KEY) key;                                                                                       \
    AMBUS_DATA_TYPE_CT(VAL) val;                                                                                       \
  };

#define AMBUS_DATA_TYPE_DEFINE_MAP(t, KEY, key, VAL, val)                                                              \
  struct ambus_data_type_info_map AMBUS_DATA_TYPE_INFO(t) = {                                                          \
      {AMBUS_DATA_TYPE_SIG(t), ambus_data_pack_map, ambus_data_unpack_map, ambus_data_free_map},                       \
      AMBUS_DATA_TYPE_INFO_PTR(KEY),                                                                                   \
      AMBUS_DATA_TYPE_INFO_PTR(VAL),                                                                                   \
      AMBUS_DATA_TYPE_SIG(KEY) AMBUS_DATA_TYPE_SIG(VAL),                                                               \
      sizeof(struct t),                                                                                                \
      offsetof(struct t, key),                                                                                         \
      offsetof(struct t, val)}

#ifdef __cplusplus
extern "C" {
#endif

struct aml_dbus;

struct ambus_vtable {
  const char *member;
  const char *signature;
  const char *result;
};

struct ambus_interface {
  const char *service;
  const char *object;
  const char *interface;
  struct ambus_vtable vtable[];
};

struct ambus_data_type_info {
  const char *signature;
  int (*pack)(struct ambus_data_type_info *t, sd_bus_message *m, void *val);
  int (*unpack)(struct ambus_data_type_info *t, sd_bus_message *m, void *val);
  void (*free)(struct ambus_data_type_info *t, void *val);
};

struct ambus_data_pack_info {
  struct ambus_data_type_info *type;
  void *val;
  int output;
};

struct ambus_data_type_info_array {
  struct ambus_data_type_info ainfo;
  struct ambus_data_type_info *dinfo;
  int size;
  int offset;
  int ptr_offset;
};

struct ambus_data_type_info_map {
  struct ambus_data_type_info ainfo;
  struct ambus_data_type_info *kinfo;
  struct ambus_data_type_info *vinfo;
  char *contents;
  int size;
  int koff;
  int voff;
};

struct ambus_data_type_info_struct {
  struct ambus_data_type_info ainfo;
  char *contents;
  int size;
  struct {
    struct ambus_data_type_info *finfo;
    int offset;
  } fields_info[];
};

AMBUS_DECLARE_DATA_TYPE(uint8_t);
AMBUS_DECLARE_DATA_TYPE(bool);
AMBUS_DECLARE_DATA_TYPE(uint16_t);
AMBUS_DECLARE_DATA_TYPE(int16_t);
AMBUS_DECLARE_DATA_TYPE(int32_t);
AMBUS_DECLARE_DATA_TYPE(uint32_t);
AMBUS_DECLARE_DATA_TYPE(int64_t);
AMBUS_DECLARE_DATA_TYPE(uint64_t);
AMBUS_DECLARE_DATA_TYPE(double);
AMBUS_DECLARE_DATA_TYPE(string);
AMBUS_DECLARE_DATA_TYPE(unixfd);

static const char *AMBUS_DEFAULT_SYSTEM = (const char *)(0);
static const char *AMBUS_DEFAULT_USER = (const char *)(1);
static const char *AMBUS_DEFAULT_NONE = (const char *)(2);
/**
 * @brief create a new dbus connection, this function MUST be called in dispatch thread (before ambus_run)
 *
 * @param address, dbus address, special addresses:
 *   NULL/AMBUS_DEFAULT_SYSTEM: default system bus
 *   AMBUS_DEFAULT_USER: default session bus
 *   AMBUS_DEFAULT_NONE: do not open dbus connection, only create event loop
 * @param mode, resolved, must be 0 by now
 *
 * @return return a pointer of opaque aml_dbus object
 */
struct aml_dbus *ambus_new(const char *address, int mode);

/**
 * @brief ensure dbus is created and dispatch thread is running
 *
 * @param ppambus, specify pointer of the bus to be ensured, and output the new created bus
 *   if NULL, it will always create new bus and dispatch thread
 *
 * @return return new created ambus if not created, else return *ppambus
 */
struct aml_dbus *ambus_ensure_run(struct aml_dbus **ppambus);

/**
 * @brief free the bus object
 *
 * @param ambus
 */
void ambus_free(struct aml_dbus *ambus);

/**
 * @brief run the event loop, dispatch dbus messages, must be called in the same thread with ambus_new
 *
 * @param ambus
 *
 * @return
 */
int ambus_run(struct aml_dbus *ambus);
sd_bus *ambus_sdbus(struct aml_dbus *ambus);
sd_event *ambus_sdevent(struct aml_dbus *ambus);
// check whether current thread is dispatch thread
bool ambus_in_dispatch_thread(struct aml_dbus *ambus);
// wait until (*val & mask) != 0
void ambus_wait_flag(struct aml_dbus *ambus, uint32_t *val, uint32_t mask);
// set flag *val |= mask and notify all waiters
void ambus_set_flag(struct aml_dbus *ambus, uint32_t *val, uint32_t mask);
// call cb(param) in dispatch thread asynchronously
int ambus_post_task(struct aml_dbus *ambus, void (*cb)(void *), void *param);
// call cb(param) in dispatch thread synchronously
int ambus_run_in_dispatch(struct aml_dbus *ambus, void (*cb)(void *), void *param);
// call cb(param) after delay_ms in dispatch thread asynchronously
int ambus_post_delay_task(struct aml_dbus *ambus, uint32_t delay_ms, void (*cb)(void *), void *param);
// call dbus method asynchronously, msgpack is used to pack input arguments of the method
int ambus_call_async_general(struct aml_dbus *ambus, const char *destination, const char *path, const char *interface,
                             const char *member, void *userdata, int (*msgpack)(sd_bus_message *m, void *userdata),
                             int (*reply_msg_handle)(sd_bus_message *m, void *userdata, sd_bus_error *ret_error));
// call dbus method synchronously, it MUST be call in a thread other than dispatch thread
int ambus_call_sync_general(struct aml_dbus *ambus, const char *destination, const char *path, const char *interface,
                            const char *member, void *userdata, int (*msgpack)(sd_bus_message *m, void *userdata),
                            int (*msgunpack)(sd_bus_message *m, void *userdata));

int ambus_data_pack_basic(struct ambus_data_type_info *t, sd_bus_message *m, void *val);
int ambus_data_unpack_basic(struct ambus_data_type_info *t, sd_bus_message *m, void *val);
int ambus_data_unpack_string(struct ambus_data_type_info *t, sd_bus_message *m, void *val);
int ambus_data_pack_array(struct ambus_data_type_info *t, sd_bus_message *m, void *val);
int ambus_data_unpack_array(struct ambus_data_type_info *t, sd_bus_message *m, void *val);
void ambus_data_free_array(struct ambus_data_type_info *t, void *val);
void ambus_data_free_struct(struct ambus_data_type_info *t, void *val);
int ambus_data_unpack_struct(struct ambus_data_type_info *t, sd_bus_message *m, void *val);
int ambus_data_pack_struct(struct ambus_data_type_info *t, sd_bus_message *m, void *val);
int ambus_data_pack_map(struct ambus_data_type_info *t, sd_bus_message *m, void *val);
int ambus_data_unpack_map(struct ambus_data_type_info *t, sd_bus_message *m, void *val);
void ambus_data_free_map(struct ambus_data_type_info *t, void *val);
int ambus_pack_all(sd_bus_message *m, struct ambus_data_pack_info *pi);
int ambus_unpack_all(sd_bus_message *m, struct ambus_data_pack_info *pi);
void ambus_free_all(struct ambus_data_pack_info *pi);
int ambus_dispatch_to_implement(sd_bus_message *m, struct ambus_data_pack_info *pi, int (*call)(void));
int ambus_call_sync_with_packer(struct aml_dbus *ambus, const char *destination, const char *path,
                                const char *interface, const char *member, struct ambus_data_pack_info *pi);

#ifdef __cplusplus
} /* extern "C" */
#include <iostream>
#include <list>
#include <string>
#include <type_traits>
#include <vector>
#include <functional>
#include <map>
#include <unordered_map>

// T shall be decay type, no const, no reference, no pointer
template <typename T, typename TEMPTY = void> struct ambus_data_type;

template <char... sigs> struct ambus_signature {
  constexpr static char value[] = {sigs..., 0};
};
template <char... sigs> constexpr char ambus_signature<sigs...>::value[];
template <typename... Args> struct ambus_signature_concat;
template <char... sig1, char... sig2>
struct ambus_signature_concat<ambus_signature<sig1...>, ambus_signature<sig2...>> {
  using type = ambus_signature<sig1..., sig2...>;
};
template <typename T1, typename... Args> struct ambus_signature_concat<T1, Args...> {
  using type = typename ambus_signature_concat<T1, typename ambus_signature_concat<Args...>::type>::type;
};

template <typename T, int sig, typename CT = T> struct ambus_simple_type {
  using signature = ambus_signature<sig>;
  static int pack(sd_bus_message *msg, const T &val) {
    const CT ct = static_cast<CT>(val);
    return sd_bus_message_append_basic(msg, sig, &ct);
  }
  static int unpack(sd_bus_message *msg, T &val) {
    CT ct;
    int r = sd_bus_message_read_basic(msg, sig, &ct);
    if (r > 0)
      val = static_cast<T>(ct);
    return r;
  }
};

template <typename T, int S = sizeof(T)> struct ambus_simple_struct {
  using signature = ambus_signature<'a', 'y'>;
  static int pack(sd_bus_message *msg, const T &val) {
    return sd_bus_message_append_array(msg, 'y', (const void *)&val, S);
  }
  static int unpack(sd_bus_message *msg, T &val) {
    const void *ptr;
    size_t size = 0;
    int r = sd_bus_message_read_array(msg, 'y', &ptr, &size);
    if (r >= 0 && size == S)
      memcpy(&val, ptr, size);
    else
      AMBUS_LOGW("fail to read array from msg, ret %d size %d sizeof(T) %d", r, (int)size, S);
    return r;
  }
};
template <typename T, int N> struct ambus_data_type<T[N]> : ambus_simple_struct<T[N]> {};

// buildin types
template <> struct ambus_data_type<char> : ambus_simple_type<char, 'y'> {};
template <> struct ambus_data_type<uint8_t> : ambus_simple_type<uint8_t, 'y'> {};
template <> struct ambus_data_type<int8_t> : ambus_simple_type<int8_t, 'y'> {};
template <> struct ambus_data_type<bool> : ambus_simple_type<bool, 'b', int> {};
template <> struct ambus_data_type<int16_t> : ambus_simple_type<int16_t, 'n'> {};
template <> struct ambus_data_type<uint16_t> : ambus_simple_type<uint16_t, 'q'> {};
template <> struct ambus_data_type<int32_t> : ambus_simple_type<int32_t, 'i'> {};
template <> struct ambus_data_type<uint32_t> : ambus_simple_type<uint32_t, 'u'> {};
template <> struct ambus_data_type<int64_t> : ambus_simple_type<int64_t, 'x'> {};
template <> struct ambus_data_type<uint64_t> : ambus_simple_type<uint64_t, 't'> {};
template <> struct ambus_data_type<double> : ambus_simple_type<double, 'd'> {};
template <typename T>
struct ambus_data_type<T, typename std::enable_if<std::is_same<T, long>::value>::type> : ambus_simple_type<T, 'x'> {};
//template <> struct ambus_data_type<const char *> : ambus_simple_type<const char *, 's'> {};
template <typename T>
struct ambus_data_type<T, typename std::enable_if<std::is_floating_point<T>::value>::type>
    : ambus_simple_type<T, 'd', double> {};
template <typename T>
struct ambus_data_type<T, typename std::enable_if<std::is_enum<T>::value>::type> : ambus_simple_type<T, 'i', int32_t> {
};
template <typename T>
struct ambus_data_type<T, typename std::enable_if<std::is_pod<T>::value && std::is_class<T>::value>::type>
    : ambus_simple_struct<T> {};
template <> struct ambus_data_type<std::string> {
  using signature = ambus_signature<'s'>;
  static int pack(sd_bus_message *msg, const std::string &val) {
    return sd_bus_message_append_basic(msg, 's', val.c_str());
  }
  static int unpack(sd_bus_message *msg, std::string &val) {
    const char *ptr = NULL;
    int r = sd_bus_message_read_basic(msg, 's', &ptr);
    if (r >= 0 && ptr)
      val = ptr;
    return r;
  }
};
template <template <class...> class C, typename T> struct ambus_data_type<C<T>> {
  using signature =
      typename ambus_signature_concat<ambus_signature<'a'>,
                                      typename ambus_data_type<typename std::decay<T>::type>::signature>::type;
  static int pack(sd_bus_message *msg, const C<T> &val) {
    int r = sd_bus_message_open_container(msg, 'a', ambus_data_type<typename std::decay<T>::type>::signature::value);
    for (auto it = val.begin(); r >= 0 && it != val.end(); ++it)
      r = ambus_data_type<typename std::decay<T>::type>::pack(msg, *it);
    sd_bus_message_close_container(msg);
    return 0;
  }
  static int unpack(sd_bus_message *msg, C<T> &val) {
    int r = sd_bus_message_enter_container(msg, 'a', ambus_data_type<typename std::decay<T>::type>::signature::value);
    T item;
    while ((r = ambus_data_type<typename std::decay<T>::type>::unpack(msg, item)) > 0)
      val.push_back(item);
    sd_bus_message_exit_container(msg);
    return 0;
  }
};

template <template <class...> class M, typename K, typename V>
struct ambus_data_type<M<K, V>, typename std::enable_if<std::is_same<M<K, V>, std::map<K, V>>::value ||
                                                        std::is_same<M<K, V>, std::unordered_map<K, V>>::value>::type> {
  using decay_val_type = typename std::decay<V>::type;
  using signature_kv = typename ambus_signature_concat<typename ambus_data_type<K>::signature,
                                                       typename ambus_data_type<decay_val_type>::signature>::type;
  using signature_map = typename ambus_signature_concat<ambus_signature<'{'>, signature_kv, ambus_signature<'}'>>::type;
  using signature = typename ambus_signature_concat<ambus_signature<'a'>, signature_map>::type;
  static int pack(sd_bus_message *msg, const M<K, V> &val) {
    int r = sd_bus_message_open_container(msg, 'a', signature_map::value);
    for (auto it = val.begin(); r >= 0 && it != val.end(); ++it) {
      if ((r = sd_bus_message_open_container(msg, 'e', signature_kv::value)) >= 0) {
        if ((r = ambus_data_type<K>::pack(msg, it->first)) >= 0)
          r = ambus_data_type<decay_val_type>::pack(msg, it->second);
        sd_bus_message_close_container(msg);
      }
    }
    sd_bus_message_close_container(msg);
    return 0;
  }
  static int unpack(sd_bus_message *msg, M<K, V> &val) {
    int r = sd_bus_message_enter_container(msg, 'a', signature_map::value);
    for (; (r = sd_bus_message_enter_container(msg, 'e', signature_kv::value)) > 0;
         sd_bus_message_exit_container(msg)) {
      K k;
      if ((r = ambus_data_type<K>::unpack(msg, k)) > 0)
        r = ambus_data_type<decay_val_type>::unpack(msg, val[k]);
    }
    sd_bus_message_exit_container(msg);
    return r;
  }
};

struct ambus_unix_fd_t {
  ambus_unix_fd_t() : fd(-1) {}
  ambus_unix_fd_t(const int f) : fd(f) {}
  operator int() const { return fd; };
  int fd;
};
template <> struct ambus_data_type<ambus_unix_fd_t> : ambus_simple_type<ambus_unix_fd_t, 'h', int> {};

// output argument must be non-const reference
template <typename T>
struct is_output_parameter
    : public std::integral_constant<bool, std::is_lvalue_reference<T>::value &&
                                              !std::is_const<typename std::remove_reference<T>::type>::value> {};

template <typename... Args> struct ambus_data_pack;

template <typename T, typename... Args> struct ambus_data_pack<T, Args...> : public ambus_data_pack<Args...> {
  constexpr static bool is_output =
      std::is_lvalue_reference<T>::value && !std::is_const<typename std::remove_reference<T>::type>::value;
  using parent_class = ambus_data_pack<Args...>;
  using decay_value_type = typename std::conditional<std::is_array<T>::value, T, typename std::decay<T>::type>::type;
  using signature_output = typename ambus_signature_concat<
      typename std::conditional<is_output, typename ambus_data_type<decay_value_type>::signature,
                                ambus_signature<>>::type,
      typename parent_class::signature_output>::type;
  using signature_input = typename ambus_signature_concat<
      typename std::conditional<!is_output, typename ambus_data_type<decay_value_type>::signature,
                                ambus_signature<>>::type,
      typename parent_class::signature_input>::type;

  static int packall(bool output, sd_bus_message *msg, const decay_value_type &val1, const Args &... vals) {
    int ret = is_output == output ? ambus_data_type<decay_value_type>::pack(msg, val1) : 0;
    return ret >= 0 ? parent_class::packall(output, msg, vals...) : ret;
  }
  static int unpackall(bool output, sd_bus_message *msg, T &val1, Args &... vals) {
    int ret = is_output == output ? ambus_data_type<decay_value_type>::unpack(msg, const_cast<decay_value_type&>(val1)) : 1;
    return ret > 0 ? parent_class::unpackall(output, msg, vals...) : ret;
  }
  decay_value_type val;
  int unpack(bool output, sd_bus_message *msg) {
    int ret = output == is_output_parameter<T>::value ? ambus_data_type<decay_value_type>::unpack(msg, val) : 0;
    return ret >= 0 ? parent_class::unpack(output, msg) : ret;
  }
  int pack(bool output, sd_bus_message *msg) {
    int ret = output == is_output_parameter<T>::value ? ambus_data_type<decay_value_type>::pack(msg, val) : 0;
    return ret >= 0 ? parent_class::pack(output, msg) : ret;
  }
  template <typename RET, typename... FArgs, typename... CArgs>
  int apply(RET &ret, RET (*pfn)(FArgs...), CArgs &... args) {
    return parent_class::apply(ret, pfn, args..., val);
  }
  template <typename TOBJ, typename RET, typename... FArgs, typename... CArgs>
  int apply_member(TOBJ *pThis, RET &ret, RET (TOBJ::*pfn)(FArgs...), CArgs &... args) {
    return parent_class::apply_member(pThis, ret, pfn, args..., val);
  }
};

template <> struct ambus_data_pack<> {
  // virtual ~ambus_data_pack<>() {}
  using signature_output = ambus_signature<>;
  using signature_input = ambus_signature<>;
  static int packall(bool output, sd_bus_message *msg) { return 0; }
  static int unpackall(bool output, sd_bus_message *msg) { return 1; }
  int unpack(bool output, sd_bus_message *msg) { return 0; }
  int pack(bool output, sd_bus_message *msg) { return 0; }
  template <typename RET, typename... FArgs, typename... CArgs>
  int apply(RET &ret, RET (*pfn)(FArgs...), CArgs &... args) {
    ret = pfn(args...);
    return 0;
  }
  template <typename TOBJ, typename RET, typename... FArgs, typename... CArgs>
  int apply_member(TOBJ *pThis, RET &ret, RET (TOBJ::*pfn)(FArgs...), CArgs &... args) {
    ret = (pThis->*pfn)(args...);
    return 0;
  }
};

#define GEN_STRUCT_FIELDS_TYPE(T, F) , decltype(T::F)
#define GEN_STRUCT_FIELDS_VAL(v, F) , v.F
#define GEN_STRUCT_FIELDS_UNPACK(T, F)                                                                                 \
  if (r >= 0)                                                                                                          \
    r = ambus_data_type<decltype(T::F)>::unpack(msg, val.F);

#define AMBUS_STRUCT_DEFINE(T, F1, ...)                                                                                \
  template <> struct ambus_data_type<T> {                                                                              \
    using fields_pack = ambus_data_pack<decltype(T::F1) MACRO_MAP(GEN_STRUCT_FIELDS_TYPE, T, __VA_ARGS__)>;            \
    using signature = typename ambus_signature_concat<ambus_signature<'('>, fields_pack::signature_input,              \
                                                      ambus_signature<')'>>::type;                                     \
    static int pack(sd_bus_message *msg, const T &val) {                                                               \
      int r = sd_bus_message_open_container(msg, 'r', fields_pack::signature_input::value);                            \
      if (r >= 0)                                                                                                      \
        r = fields_pack::packall(false, msg, val.F1 MACRO_MAP(GEN_STRUCT_FIELDS_VAL, val, __VA_ARGS__));               \
      sd_bus_message_close_container(msg);                                                                             \
      return r;                                                                                                        \
    }                                                                                                                  \
    static int unpack(sd_bus_message *msg, T &val) {                                                                   \
      int r = sd_bus_message_enter_container(msg, 'r', fields_pack::signature_input::value);                           \
      if (r >= 0)                                                                                                      \
        r = fields_pack::unpackall(false, msg, val.F1 MACRO_MAP(GEN_STRUCT_FIELDS_VAL, val, __VA_ARGS__));             \
      sd_bus_message_exit_container(msg);                                                                              \
      return r;                                                                                                        \
    }                                                                                                                  \
  }

template <typename... Args> struct ambus_method_call;
template <> struct ambus_method_call<> : public ambus_vtable {
  ambus_method_call<>(const char *_name, const char *_sig, const char *_res) {
    member = _name;
    signature = _sig;
    result = _res;
  }
  static int handler(sd_bus_message *m, void *userdata, sd_bus_error *ret_error) {
    struct ambus_method_call<> *pcall = (struct ambus_method_call<> *)userdata;
    return pcall->call(m);
  }
  virtual int call(sd_bus_message *m) { return 0; };
};
template <typename RET, typename CLS, typename... Args>
struct ambus_method_call<RET (CLS::*)(Args...)> : public ambus_method_call<> {
  using data_pack = ambus_data_pack<Args...>;
  using signature_output = typename ambus_signature_concat<typename ambus_data_type<RET>::signature,
                                                           typename data_pack::signature_output>::type;
  ambus_method_call<RET (CLS::*)(Args...)>(CLS *_this, const char *name, RET (CLS::*pfn)(Args...))
      : pThis(_this),
        callback(pfn), ambus_method_call<>(name, data_pack::signature_input::value, signature_output::value) {}
  virtual int call(sd_bus_message *m) {
    data_pack val;
    int r = 0;
    if (signature[0])
      r = val.unpack(false, m);
    RET ret;
    if (r >= 0)
      r = val.apply_member(pThis, ret, callback);
    sd_bus_message *reply = NULL;
    if (r >= 0)
      r = sd_bus_message_new_method_return(m, &reply);
    if (r >= 0)
      r = ambus_data_type<RET>::pack(reply, ret);
    if (r >= 0 && result[0])
      r = val.pack(true, reply);
    if (r >= 0)
      r = sd_bus_send(sd_bus_message_get_bus(m), reply, NULL);
    if (reply)
      sd_bus_message_unref(reply);
    return r;
  }
  RET (CLS::*callback)(Args...);
  CLS *pThis;
};

template <typename RET, typename... Args> struct ambus_method_call<RET (*)(Args...)> : public ambus_method_call<> {
  using data_pack = ambus_data_pack<Args...>;
  using signature_output = typename ambus_signature_concat<typename ambus_data_type<RET>::signature,
                                                           typename data_pack::signature_output>::type;
  ambus_method_call<RET (*)(Args...)>(const char *name, RET (*pfn)(Args...))
      : callback(pfn), ambus_method_call<>(name, data_pack::signature_input::value, signature_output::value) {}
  virtual int call(sd_bus_message *m) {
    data_pack val;
    int r = 0;
    if (signature[0])
      r = val.unpack(false, m);
    RET ret;
    if (r >= 0)
      r = val.apply(ret, callback);
    sd_bus_message *reply = NULL;
    if (r >= 0)
      r = sd_bus_message_new_method_return(m, &reply);
    if (r >= 0)
      r = ambus_data_type<RET>::pack(reply, ret);
    if (r >= 0 && result[0])
      r = val.pack(true, reply);
    if (r >= 0)
      r = sd_bus_send(sd_bus_message_get_bus(m), reply, NULL);
    if (reply)
      sd_bus_message_unref(reply);
    return r;
  }
  RET (*callback)(Args...);
};

struct ambus_runnable_task {
  using prototype = std::function<void(void)>;
  prototype func;
  ambus_runnable_task(prototype &&f) : func(f){};
  static void cb(void *p) { static_cast<ambus_runnable_task *>(p)->func(); };
  int run_on(struct aml_dbus *ambus) { return ambus_run_in_dispatch(ambus, ambus_runnable_task::cb, this); }
};

template <typename T> struct ambus_method_proxy;
struct ambus_method_proxy_packer {
  std::function<int(sd_bus_message *)> pk, upk;
  static int pack(sd_bus_message *m, void *userdata) { return ((ambus_method_proxy_packer *)userdata)->pk(m); }
  static int unpack(sd_bus_message *m, void *userdata) { return ((ambus_method_proxy_packer *)userdata)->upk(m); }
};
template <typename RET, typename... Args> struct ambus_method_proxy<RET (*)(Args...)> {
  static RET call(struct aml_dbus *ambus, const char *service, const char *object, const char *interface,
                  const char *member, Args... args) {
    RET ret;
    ambus_method_proxy_packer p = {
        [&](sd_bus_message *msg) { return ambus_data_pack<Args...>::packall(false, msg, args...); },
        [&](sd_bus_message *msg) { return ambus_data_pack<RET &, Args...>::unpackall(true, msg, ret, args...); }};
    int r = ambus_call_sync_general(ambus, service, object, interface, member, &p, ambus_method_proxy_packer::pack,
                                    ambus_method_proxy_packer::unpack);
    return ret;
  }
};

#endif

#endif /* end of include guard: AML_DBUS_H */
