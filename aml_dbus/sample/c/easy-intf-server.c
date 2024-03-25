#include "aml-dbus.h"
#include "easy-intf.h"

AMBUS_DATA_TYPE_DEFINE_ARRAY(TestArray, string);
AMBUS_DATA_TYPE_DEFINE_STRUCT(TestStruct, int32_t, ival, double, dval, string, sval);
AMBUS_DATA_TYPE_DEFINE_MAP(TestMap, string, key, int32_t, val);

//static int test1(int32_t i1, int32_t * o1)
static GEN_API_DECLARE(P, test1, int32_t, i1, OUT(int32_t), o1) {
  *o1 = i1 * 2;
  printf("in test1(%d,>%d)\n", i1, *o1);
  return 0;
}

//static int test2(char *s1, char **s2)
static GEN_API_DECLARE(P, test2, string, s1, OUT(string), s2) {
  sprintf(*s2 = malloc(256), "return '%.240s'", s1);
  printf("in test2(%s,>%s)\n", s1, *s2);
  return 0;
}

// static GEN_API_DECLARE(P, test3, TestArray, a1, OUT(TestArray), a2)
static int test3(struct TestArray *a1, struct TestArray **a2) {
  struct TestArray *ret = AMBUS_ARRAY_NEW(TestArray, a1->num);
  for (int i = 0; i < a1->num; i++) {
    printf("%d:%s\n", i, a1->val[i]);
    ret->val[i] = strdup(a1->val[i]);
  }
  *a2 = ret;
  return 0;
}

// static GEN_API_DECLARE(P, test4, TestStruct, s1, OUT(TestStruct), s2)
static int test4(struct TestStruct *s1, struct TestStruct **s2) {
  printf("%d %f %s\n", s1->ival, s1->dval, s1->sval);
  struct TestStruct *ret = malloc(sizeof(*ret));
  ret->ival = s1->ival;
  ret->dval = s1->dval;
  ret->sval = strdup(s1->sval);
  *s2 = ret;
  return 0;
}

//static GEN_API_DECLARE(P, test5, TestMap, m1, OUT(TestMap), m2)
static int test5(struct TestMap* m1, struct TestMap** m2) {
  struct TestMap *ret = NULL, **ptail = &ret;
  printf("{");
  for (struct TestMap *p = m1; p != NULL; p = p->next) {
    if (p != m1)
      printf(",");
    printf("\"%s\":%d", p->key, p->val);
    struct TestMap *c = malloc(sizeof(*c));
    c->key = strdup(p->key);
    c->val = p->val;
    *ptail = c;
    ptail = &c->next;
  }
  printf("}\n");
  *ptail = NULL;
  *m2 = ret;
  return 0;
}


AMBUS_DEFINE_INTERFACE_EX(SAMPLE_INTF, SAMPLE_SERVICE, SAMPLE_OBJECT, SAMPLE_INTERFACE);
AMBUS_DEFINE_VTABLE_EX(SAMPLE_INTF);

int main(int argc, char *argv[])
{
  int r;
  struct aml_dbus *ambus = ambus_new(NULL, 0);
  AMBUS_CHECK(r, AMBUS_ADD_VTABLE(ambus, SAMPLE_INTF, NULL), );
  AMBUS_CHECK(r, AMBUS_REQUEST_NAME(ambus, SAMPLE_INTF), );
  ambus_run(ambus);
  return 0;
}
