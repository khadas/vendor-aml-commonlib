#ifndef SAMPLE_INTF_H

#define SAMPLE_INTF_H

#define SAMPLE_SERVICE "amlogic.yocto.test"
#define SAMPLE_OBJECT "/amlogic/yocto/test/obj1"
#define SAMPLE_INTERFACE "amlogic.yocto.test.intf1"

#define SAMPLE_INTF(P, PROP, RROPRW, METHOD, SIGNAL)                                                                   \
  METHOD(P, test1, int32_t, i1, OUT(int32_t), o1)                                                                      \
  METHOD(P, test2, string, s1, OUT(string), s2)                                                                        \
  METHOD(P, test3, TestArray, a1, OUT(TestArray), a2)                                                                  \
  METHOD(P, test4, TestStruct, s1, OUT(TestStruct), s2)                                                                \
  METHOD(P, test5, TestMap, m1, OUT(TestMap), m2)

// customer types
#define AMBUS_DATA_TYPE_TestArray "as"
#define AMBUS_DATA_TYPE_TestStruct "(ids)"
#define AMBUS_DATA_TYPE_TestMap "a{si}"
AMBUS_DATA_TYPE_DECLARE_ARRAY(TestArray, string);
AMBUS_DATA_TYPE_DECLARE_STRUCT(TestStruct, int32_t, ival, double, dval, string, sval);
AMBUS_DATA_TYPE_DECLARE_MAP(TestMap, string, key, int32_t, val);

#endif /* end of include guard: SAMPLE_INTF_H */
