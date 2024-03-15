#ifndef AMBUS_GENERATOR
#define AMBUS_GENERATOR
#endif

#undef AMBUS_BEGIN_INTF
#undef AMBUS_PROP
#undef AMBUS_PROPRW
#undef AMBUS_METHOD
#undef AMBUS_SIGNAL
#undef AMBUS_END_INTF
#define AMBUS_BEGIN_INTF(SERVICE, OBJ, INTF) enum {
#define AMBUS_PROP(_member, _signature) MACRO_CAT2(AMBUS_INTERFACE_ID, _member##_enum),
#define AMBUS_PROPRW(_member, _signature) MACRO_CAT2(AMBUS_INTERFACE_ID, _member##_enum),
#define AMBUS_METHOD(_member, _signature, _result) MACRO_CAT2(AMBUS_INTERFACE_ID, _member##_enum),
#define AMBUS_SIGNAL(_member, _signature) MACRO_CAT2(AMBUS_INTERFACE_ID, _member##_enum),
#define AMBUS_END_INTF()                                                                                               \
  MACRO_CAT2(AMBUS_INTERFACE_ID, _END)                                                                                 \
  }                                                                                                                    \
  ;
#include AMBUS_INTERFACE_FILE

#undef AMBUS_BEGIN_INTF
#undef AMBUS_PROP
#undef AMBUS_PROPRW
#undef AMBUS_METHOD
#undef AMBUS_SIGNAL
#undef AMBUS_END_INTF
#define AMBUS_BEGIN_INTF(SERVICE, OBJ, INTF) extern struct ambus_interface MACRO_CAT2(AMBUS_INTERFACE_ID, _interface);
#define AMBUS_PROP(_member, _signature)
#define AMBUS_PROPRW(_member, _signature)
#define AMBUS_METHOD(_member, _signature, _result)
#define AMBUS_SIGNAL(_member, _signature)
#define AMBUS_END_INTF()
#include AMBUS_INTERFACE_FILE

#ifdef AMBUS_GENERATE_INTERFACE
#undef AMBUS_BEGIN_INTF
#undef AMBUS_PROP
#undef AMBUS_PROPRW
#undef AMBUS_METHOD
#undef AMBUS_SIGNAL
#undef AMBUS_END_INTF
#define AMBUS_BEGIN_INTF(SERVICE, OBJ, INTF)                                                                           \
  struct ambus_interface MACRO_CAT2(AMBUS_INTERFACE_ID, _interface) = {SERVICE, OBJ, INTF, {
#define AMBUS_PROP(_member, _signature) {#_member, _signature, NULL},
#define AMBUS_PROPRW(_member, _signature) {#_member, _signature, NULL},
#define AMBUS_METHOD(_member, _signature, _result) {#_member, _signature, _result},
#define AMBUS_SIGNAL(_member, _signature) {#_member, _signature, NULL},
#define AMBUS_END_INTF()                                                                                               \
  { NULL, NULL, NULL }                                                                                                 \
  }                                                                                                                    \
  }                                                                                                                    \
  ;
#include AMBUS_INTERFACE_FILE
#endif // AMBUS_GENERATE_INTERFACE

#ifdef AMBUS_GENERATE_VTABLE
#undef AMBUS_BEGIN_INTF
#undef AMBUS_PROP
#undef AMBUS_PROPRW
#undef AMBUS_METHOD
#undef AMBUS_SIGNAL
#undef AMBUS_END_INTF
#define AMBUS_BEGIN_INTF(SERVICE, OBJ, INTF)
#define AMBUS_PROP(_member, _signature)                                                                                \
  static int MACRO_CAT2(AMBUS_INTERFACE_ID, _property_get)(sd_bus * bus, const char *path, const char *interface,      \
                                                           const char *property, sd_bus_message *reply,                \
                                                           void *userdata, sd_bus_error *ret_error);
#define AMBUS_PROPRW(_member, _signature)                                                                              \
  static int MACRO_CAT2(AMBUS_INTERFACE_ID, _property_get)(sd_bus * bus, const char *path, const char *interface,      \
                                                           const char *property, sd_bus_message *reply,                \
                                                           void *userdata, sd_bus_error *ret_error);                   \
  static int MACRO_CAT2(AMBUS_INTERFACE_ID, _property_set)(sd_bus * bus, const char *path, const char *interface,      \
                                                           const char *property, sd_bus_message *value,                \
                                                           void *userdata, sd_bus_error *ret_error);
#define AMBUS_METHOD(_member, _signature, _result)                                                                     \
  static int MACRO_CAT2(AMBUS_INTERFACE_ID, _method_##_member)(sd_bus_message * m, void *userdata,                     \
                                                               sd_bus_error *ret_error);
#define AMBUS_SIGNAL(_member, _signature)
#define AMBUS_END_INTF()
#include AMBUS_INTERFACE_FILE

#undef AMBUS_BEGIN_INTF
#undef AMBUS_PROP
#undef AMBUS_PROPRW
#undef AMBUS_METHOD
#undef AMBUS_SIGNAL
#undef AMBUS_END_INTF
#define AMBUS_BEGIN_INTF(SERVICE, OBJ, INTF)                                                                           \
  static const sd_bus_vtable MACRO_CAT2(AMBUS_INTERFACE_ID, _vtable)[] = {SD_BUS_VTABLE_START(0),
#define AMBUS_PROP(_member, _signature)                                                                                \
  SD_BUS_PROPERTY(#_member, _signature, MACRO_CAT2(AMBUS_INTERFACE_ID, _property_get), 0, 0),
#define AMBUS_PROPRW(_member, _signature)                                                                              \
  SD_BUS_WRITABLE_PROPERTY(#_member, _signature, MACRO_CAT2(AMBUS_INTERFACE_ID, _property_get),                        \
                           MACRO_CAT2(AMBUS_INTERFACE_ID, _property_set), 0, 0),
#define AMBUS_METHOD(_member, _signature, _result)                                                                     \
  SD_BUS_METHOD(#_member, _signature, _result, MACRO_CAT2(AMBUS_INTERFACE_ID, _method_##_member),                      \
                SD_BUS_VTABLE_UNPRIVILEGED),
#define AMBUS_SIGNAL(_member, _signature) SD_BUS_SIGNAL(#_member, _signature, 0),
#define AMBUS_END_INTF()                                                                                               \
  SD_BUS_VTABLE_END                                                                                                    \
  }                                                                                                                    \
  ;
#include AMBUS_INTERFACE_FILE
#endif // AMBUS_GENERATE_VTABLE

#undef AMBUS_BEGIN_INTF
#undef AMBUS_PROP
#undef AMBUS_PROPRW
#undef AMBUS_METHOD
#undef AMBUS_SIGNAL
#undef AMBUS_END_INTF
#undef AMBUS_GENERATOR

