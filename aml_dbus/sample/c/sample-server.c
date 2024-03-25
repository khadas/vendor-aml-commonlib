#include "aml-dbus.h"
#include <errno.h>

#define TEST_SERVICE "amlogic.yocto.test"
#define TEST_OBJECT "/amlogic/yocto/test/obj1"
#define TEST_INTERFACE "amlogic.yocto.test.intf1"
#define TEST_INTF(p, PROP, RROPRW, METHOD, SIGNAL)                                                                     \
  PROP(p, Count, "i")                                                                                                  \
  METHOD(p, TestBasicType, "ybnqiuxtds", "ybnqiuxtds")                                                                 \
  METHOD(p, TestArray, "a(is)", "a(si)")                                                                               \
  SIGNAL(p, Changed, "i")

AMBUS_DECLARE_INTERFACE(TEST_INTF);
//AMBUS_DEFINE_INTERFACE(TEST_INTF, TEST_SERVICE, TEST_OBJECT, TEST_INTERFACE);
AMBUS_DEFINE_INTERFACE(TEST_INTF, NULL, NULL, NULL);

struct TestObject {
  int count;
};

static int TEST_INTF_property_get(sd_bus *bus, const char *path, const char *interface, const char *property,
                                  sd_bus_message *reply, void *userdata, sd_bus_error *ret_error) {
  struct TestObject *o = (struct TestObject *)userdata;
  if (strcmp(property, "Count") == 0) {
    return sd_bus_message_append(reply, "i", o->count);
  }
  return -EINVAL;
}

static int TEST_INTF_method_TestBasicType(sd_bus_message *m, void *userdata, sd_bus_error *ret_error) {
  struct TestObject *o = (struct TestObject *)userdata;
  uint8_t y;
  int b;
  int16_t n;
  uint16_t q;
  int32_t i;
  uint32_t u;
  int64_t x;
  uint64_t t;
  double d;
  char *s;
  int r = sd_bus_message_read(m, "ybnqiuxtds", &y, &b, &n, &q, &i, &u, &x, &t, &d, &s);
  o->count++;
  sd_bus_emit_signal(sd_bus_message_get_bus(m), sd_bus_message_get_path(m), sd_bus_message_get_interface(m), "Changed",
                     "i", o->count);
  y++;
  b=!b;
  n++;
  q++;
  i++;
  u++;
  x++;
  t++;
  d+=1;
  s+=1;
  return sd_bus_reply_method_return(m,"ybnqiuxtds",y, b, n, q, i, u, x, t, d, s);
}

static int TEST_INTF_method_TestArray(sd_bus_message *m, void *userdata, sd_bus_error *ret_error) {
  struct TestObject *o = (struct TestObject *)userdata;
  int r;
  int i;
  char *s;
  sd_bus_message *reply;
  AMBUS_CHECK(r, sd_bus_message_enter_container(m, 'a', "(is)"),);
  AMBUS_CHECK(r, sd_bus_message_new_method_return(m, &reply),);
  AMBUS_CHECK(r, sd_bus_message_open_container(reply, 'a', "(si)"),);
  while ((r = sd_bus_message_read(m, "(is)", &i, &s)) > 0) {
    AMBUS_CHECK(r,sd_bus_message_append(reply, "(si)", s, i),);
  }
  sd_bus_message_exit_container(m);
  sd_bus_message_close_container(reply);
  r = sd_bus_send(sd_bus_message_get_bus(m), reply, NULL);
  sd_bus_message_unref(reply);
  return r;
}

AMBUS_DEFINE_VTABLE(TEST_INTF);

static int call_done_TestBasicType(sd_bus_message *m, void *userdata, sd_bus_error *ret_error){
  uint8_t y;
  int b;
  int16_t n;
  uint16_t q;
  int32_t i;
  uint32_t u;
  int64_t x;
  uint64_t t;
  double d;
  char *s;
  struct ambus_vtable *mem = &AMBUS_VTABLE(TEST_INTF, TestBasicType);
  int r = sd_bus_message_read(m, mem->result, &y, &b, &n, &q, &i, &u, &x, &t, &d, &s);
  printf("call TestBasicType return y:%d b:%d n:%d q:%d i:%d u:%u x:%" PRId64 " t:%" PRIu64 " d:%f s:%s\n", y, b, n, q,
         i, u, x, t, d, s);
  // the memory of 's' string is point to somewhere in sd_bus_message, is is invalid when sd_bus_message is unrefed
  // you can sd_bus_message_ref to keep the memory of sd_bus_message, but don't forget to sd_bus_message_unref
  return 0;
}

static int test_call_async(struct aml_dbus *ambus) {
  struct ambus_vtable *mem = &AMBUS_VTABLE(TEST_INTF, TestBasicType);
  int r = 0;
  uint8_t y = 1;
  int b = false;
  int16_t n = 2;
  uint16_t q = 3;
  int32_t i = 4;
  uint32_t u = 5;
  int64_t x = 6;
  uint64_t t = 7;
  double d = 8.123;
  char *s = "aaaaa";
  int pack(sd_bus_message * m, void *userdata) {
    return sd_bus_message_append(m, mem->signature, y, b, n, q, i, u, x, t, d, s);
  }
  ambus_call_async_general(ambus, AMBUS_SERV_OBJ_INTF(TEST_INTF), "TestBasicType", NULL, pack, call_done_TestBasicType);
}

int main(int argc, char *argv[])
{
  struct TestObject obj;
  memset(&obj, 0, sizeof(obj));
  int r = 0;
  struct aml_dbus *ambus = ambus_new(NULL, 0);
  AMBUS_SERVICE(TEST_INTF) = TEST_SERVICE;
  AMBUS_OBJECT(TEST_INTF) = TEST_OBJECT;
  AMBUS_INTERFACE(TEST_INTF) = TEST_INTERFACE;
  AMBUS_CHECK(r, AMBUS_ADD_VTABLE(ambus, TEST_INTF, &obj), goto error);
  AMBUS_CHECK(r, AMBUS_REQUEST_NAME(ambus, TEST_INTF), goto error);
  test_call_async(ambus);
  ambus_run(ambus);
error:
  ambus_free(ambus);
  return 0;
}
