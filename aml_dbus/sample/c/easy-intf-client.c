#include "aml-dbus.h"
#include "easy-intf.h"

static struct aml_dbus *call_bus;

AMBUS_DATA_TYPE_DEFINE_ARRAY(TestArray, string);
AMBUS_DATA_TYPE_DEFINE_STRUCT(TestStruct, int32_t, ival, double, dval, string, sval);
AMBUS_DATA_TYPE_DEFINE_MAP(TestMap, string, key, int32_t, val);

AMBUS_DEFINE_INTERFACE_EX(SAMPLE_INTF, SAMPLE_SERVICE, SAMPLE_OBJECT, SAMPLE_INTERFACE);

// generate proxy code for SAMPLE_INTF
#define CALL_PARAM() call_bus, AMBUS_SERV_OBJ_INTF(SAMPLE_INTF)
AMBUS_DEFINE_PROXY_EX(SAMPLE_INTF, CALL_PARAM);

// define a proxy function to property method
#define CALL_PARAM_PROP() call_bus, "amlogic.yocto.Property1", "/amlogic/yocto/Property1", "amlogic.yocto.Property1"
GEN_API_METHOD_PROXY_NAME(CALL_PARAM_PROP, prop_get, get, string, key, OUT(bool), ret, OUT(string), value);

// customer type
AMBUS_DATA_TYPE_DECLARE_MAP(PropList, string, key, string, value);
#define AMBUS_DATA_TYPE_PropList "a{ss}"
AMBUS_DATA_TYPE_DEFINE_MAP(PropList, string, key, string, value);

GEN_API_METHOD_PROXY_NAME(CALL_PARAM_PROP, prop_list, list, OUT(PropList), value);

int main(int argc, char *argv[]) {
  int r;
  call_bus = ambus_ensure_run(NULL);
  int32_t o1;
  r = test1(123, &o1);
  printf("call test1(123,>%d) return %d\n", o1, r);

  char *s2 = NULL; // init NULL for output string, we need to free the output memory
  r = test2("abcdedf", &s2);
  printf("call test2(>%s) return %d\n", s2, r);
  if (s2)
    free(s2);

  static struct TestArray a1 = {3, {"123", "456", "789"}};
  struct TestArray *a2 = NULL;
  r = test3(&a1, &a2);
  printf("call test3 return %d, a2=%p\n", r, a2);
  if (a2) {
    for (int i = 0; i < a2->num; i++) {
      printf("%d:%s\n", i, a2->val[i]);
    }
    AMBUS_DATA_FREE(TestArray, &a2);
  }

  struct TestStruct s3 = {123, 456.789, "abcdefg"}, *s4 = NULL;
  r = test4(&s3, &s4);
  printf("call test4 return %d, s4=%p\n", r, s4);
  if (s4) {
    printf("(%d %f %s)\n", s4->ival, s4->dval, s4->sval);
    AMBUS_DATA_FREE(TestStruct, &s4);
  }

  struct TestMap map3 = {NULL, "k3", 123};
  struct TestMap map2 = {&map3, "k2", 456};
  struct TestMap map1 = {&map2, "k1", 789}, *map4 = NULL;
  r = test5(&map1, &map4);
  printf("call test5 return %d, map4=%p\n", r, map4);
  if (map4) {
    for (struct TestMap *p = map4; p != NULL; p = p->next) {
      printf("key %s, val %d\n", p->key, p->val);
    }
    AMBUS_DATA_FREE(TestMap, &map4);
  }

  int ret;   // use int for dbus bool type
  s2 = NULL; // init NULL for output string, we need to free the output memory
  r = prop_get("ro.launcher.apps", &ret, &s2);
  if (r >= 0 && s2) {
    printf("%d %s return %d\n", ret, s2, r);
    free(s2);
  }

  struct PropList *props = NULL;
  prop_list(&props);
  for (struct PropList *p = props; p != NULL; p = p->next) {
    printf("%s=%s\n", p->key, p->value);
  }
  AMBUS_DATA_FREE(PropList, &props);
  return 0;
}

