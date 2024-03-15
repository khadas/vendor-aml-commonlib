LDLIBS += -lsystemd -lpthread
LINK:=$(CC)

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

TARGET := $(BUILDDIR)/aml-dbus-test
$(eval $(call build_project,exe,aml-dbus-test.cpp aml-dbus.c))

TARGET := $(BUILDDIR)/ambus-sample-server
$(eval $(call build_project,exe,sample-server.c aml-dbus.c))

TARGET := $(BUILDDIR)/ambus-sample-client
$(eval $(call build_project,exe,sample-client.c aml-dbus.c))

