LDLIBS += -lsystemd -lpthread
LDFLAGS+=-L$(BUILDDIR)
LINK:=$(CC)
CPPFLAGS+=-I./ -fPIC -g

LIB_VER=0

# BUILDDIR used for out-of-source compile
BUILDDIR?=./

all:

.PHONY: clean

define build_project
OBJ_C := $(patsubst %.c,$(TARGET)_o/%.o,$(filter %.c,$2))
OBJ_CXX := $(patsubst %.cpp,$(TARGET)_o/%.o,$(filter %.cpp,$2))
OBJ_PATH := $(addprefix $(TARGET)_o/, $(sort $(dir $2)))
_dummy := $$(shell mkdir -p $$(OBJ_PATH))
$(TARGET):OBJ_ALL:=$$(OBJ_C) $$(OBJ_CXX)
$(TARGET):LINK:=$(if $(filter %.cpp,$2),$(CXX),$(CC))
$(TARGET)_clean:CLEAN_FILES:=$(TARGET) $$(OBJ_PATH) $(TARGET)_o
ifneq (,$(findstring shared_lib,$1))
ifneq ($(suffix $(TARGET)),.so)
all:$(basename $(TARGET))
$(basename $(TARGET)):$(TARGET)
	ln -frs $$< $$@
endif
endif
$(TARGET): $$(OBJ_C) $$(OBJ_CXX)
ifneq (,$(findstring shared_lib,$1))
	$$(LINK) $$(CPPFLAGS) $$(LDFLAGS) -shared -Wl,-soname,$(notdir $(TARGET)) -o $$@ $$(OBJ_ALL) $$(LDLIBS)
else ifneq (,$(findstring static_lib,$1))
	$(AR) rcs -o $$@ $$+
	$(RANLIB) $$@
else
	$$(LINK) $$(CPPFLAGS) $$(LDFLAGS) -o $$@ $$(OBJ_ALL) $$(LDLIBS)
endif

$$(OBJ_C):$(TARGET)_o/%.o:%.c
	$$(CC) $$(CPPFLAGS) $$(CFLAGS) -c $$< -o $$@

$$(OBJ_CXX):$(TARGET)_o/%.o:%.cpp
	$$(CXX) $$(CPPFLAGS) $$(CXXFLAGS) -c $$< -o $$@

.PHONY: $(TARGET)_clean

$(TARGET)_clean:
	-rm -rf $$(abspath $$(CLEAN_FILES))

clean: $(TARGET)_clean

all:$(TARGET)

endef

TARGET := $(BUILDDIR)/libamldbus.so.$(LIB_VER)
$(eval $(call build_project,shared_lib,aml-dbus.c))

TARGET := $(BUILDDIR)/ambus-sample1
$(TARGET):$(BUILDDIR)/libamldbus.so
$(TARGET):LDLIBS+=-lamldbus
#$(eval $(call build_project,exe,sample/cpp/sample1.cpp))

TARGET := $(BUILDDIR)/ambus-sample-server-1
$(TARGET):$(BUILDDIR)/libamldbus.so
$(TARGET):LDLIBS+=-lamldbus
#$(eval $(call build_project,exe,sample/c/sample-server.c))

TARGET := $(BUILDDIR)/ambus-sample-client-1
$(TARGET):$(BUILDDIR)/libamldbus.so
$(TARGET):LDLIBS+=-lamldbus
#$(eval $(call build_project,exe,sample/c/sample-client.c))

TARGET := $(BUILDDIR)/ambus-easy-intf-server
$(TARGET):$(BUILDDIR)/libamldbus.so
$(TARGET):LDLIBS+=-lamldbus
$(eval $(call build_project,exe,sample/c/easy-intf-server.c))

TARGET := $(BUILDDIR)/ambus-easy-intf-client
$(TARGET):$(BUILDDIR)/libamldbus.so
$(TARGET):LDLIBS+=-lamldbus
$(eval $(call build_project,exe,sample/c/easy-intf-client.c))

