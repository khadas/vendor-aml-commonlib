#include "aml-dbus.h"


#define TEST_SERVICE "amlogic.yocto.test"
#define TEST_OBJECT "/amlogic/yocto/test/obj1"
#define TEST_INTERFACE "amlogic.yocto.test.intf1"
#define TEST_INTF(p, PROP, RROPRW, METHOD, SIGNAL)                                                                     \
  PROP(p, Count, "i")                                                                                                  \
  METHOD(p, TestBasicType, "ybnqiuxtds", "ybnqiuxtds")                                                                 \
  METHOD(p, TestArray, "a(is)", "a(si)")                                                                               \
  SIGNAL(p, Changed, "i")

AMBUS_DECLARE_INTERFACE(TEST_INTF);
AMBUS_DEFINE_INTERFACE(TEST_INTF, TEST_SERVICE, TEST_OBJECT, TEST_INTERFACE);

#define AMBUS_SERV_OBJ_INTF_MEM(_intf, _mem) AMBUS_SERV_OBJ_INTF(_intf), #_mem

// we use two bus, one for method, one for signal
static struct aml_dbus *bus_method;
static struct aml_dbus *bus_signal;

static int dbus_signal_handle(sd_bus_message *message, void *userdata, sd_bus_error *error) {
  const char *mem = sd_bus_message_get_member(message);
  if (strcmp(mem, "Changed") == 0) {
    int v;
    sd_bus_message_read_basic(message, 'i', &v);
    //printf("got signal Changed, %d\n", v);
  }
  return 0;
}

#define MACRO_GET_2ND_(a, b, ...) b
#define MACRO_GET_2ND(...) MACRO_GET_2ND_(__VA_ARGS__)
#define AMBUS_DATA_PACK_BASIC(msg, type, ptr) sd_bus_message_append_basic(msg, type, ptr)
#define AMBUS_DATA_UNPACK_BASIC(msg, type, ptr) sd_bus_message_read_basic(msg, type, ptr)
#define AMBUS_DATA_TYPE_y uint8_t, AMBUS_DATA_PACK_BASIC, AMBUS_DATA_UNPACK_BASIC

static int call_TestBasicType(uint8_t y, int b, int16_t n, uint16_t q, int32_t i, uint32_t u, int64_t x, uint64_t t,
                              double d, char *s, uint8_t *oy, int *ob, int16_t *on, uint16_t *oq, int32_t *oi,
                              uint32_t *ou, int64_t *ox, uint64_t *ot, double *od, char **os) {
  struct ambus_vtable *mem = &AMBUS_VTABLE(TEST_INTF, TestBasicType);
  int r = 0;
  int pack(sd_bus_message * m, void *userdata) {
    r = sd_bus_message_append(m, mem->signature, y, b, n, q, i, u, x, t, d, s);
    return r;
  }
  int unpack(sd_bus_message * m, void *userdata) {
    char *ts = NULL;
    r = sd_bus_message_read(m, mem->result, oy, ob, on, oq, oi, ou, ox, ot, od, &ts);
    if (r >= 0 && ts && os) // string type memory is in sd_bus_message, which becomes invalid after this function return
      *os = strdup(ts);
    return r;
  }
  return ambus_call_sync_general(bus_method, AMBUS_SERV_OBJ_INTF_MEM(TEST_INTF, TestBasicType), NULL, pack, unpack);
}

static int test_dbus_call() {
  uint8_t y = 1;
  int b = false;
  int16_t n = 2;
  uint16_t q = 3;
  int32_t i = 4;
  uint32_t u = 5;
  int64_t x = 6;
  uint64_t t = 7;
  double d = 8.123;
  char buf[32];
  char *s = buf;
  for (int ii = 0; ii < 3; ii++) {
    sprintf(s = buf, "test%d", i);
    int r = call_TestBasicType(y, b, n, q, i, u, x, t, d, s, &y, &b, &n, &q, &i, &u, &x, &t, &d, &s);
    printf("call TestBasicType return y:%d b:%d n:%d q:%d i:%d u:%u x:%" PRId64 " t:%" PRIu64 " d:%f s:%s\n", y, b, n,
           q, i, u, x, t, d, s);
    free(s);
  }
}

int main(int argc, char *argv[]) {
  bus_method = ambus_ensure_run(NULL);
  bus_signal = ambus_ensure_run(NULL);

  char match[512];
  snprintf(match, sizeof(match), "type='signal',sender='%s',path='%s',interface='%s'", AMBUS_SERV_OBJ_INTF(TEST_INTF));
  int r = 0;
  void installcb(void *param) { r = sd_bus_add_match(ambus_sdbus(bus_signal), NULL, match, dbus_signal_handle, NULL); }
  // make sd_bus_add_match be called in bus_signal dispatch thread
  ambus_run_in_dispatch(bus_signal, installcb, NULL);
  test_dbus_call();
  ambus_run(bus_signal);
  return 0;
}
