LOCAL_PATH := $(shell pwd)

LOCAL_INCLUDES := -I. -Iinclude -I$(STAGING_DIR)/usr/include
LOCAL_LIBS := -L$(STAGING_DIR)/usr/lib -lpthread
LOCAL_DEFINES := -DHAVE_PTHREADS -DHAVE_ANDROID_OS=1

CFLAGS += -fPIC -Wall -Wno-unknown-pragmas -Wno-format -O3 -g -Wno-error=format-security
CFLAGS += $(LOCAL_INCLUDES) $(LOCAL_DEFINES) $(CFLAGS_EXTRA)

LDFLAGS += $(LOCAL_LIBS)

# binder src and obj
BINDER_CPP_SRCS := $(wildcard binder/*.cpp)
BINDER_CPP_SRCS += $(wildcard utils/*.cpp)
BINDER_C_SRCS := $(wildcard cutils/*.c)

BINDER_CPP_OBJS := $(patsubst %.cpp, %.lo, $(BINDER_CPP_SRCS))
BINDER_C_OBJS := $(patsubst %.c, %.o, $(BINDER_C_SRCS))
BINDER_OBJS := $(BINDER_C_OBJS) $(BINDER_CPP_OBJS)

# servicemanager src and obj
SVCMGR_SRCS := $(wildcard servicemgr/*.c)
SVCMGR_OBJS := $(patsubst %.c, %.o, $(SVCMGR_SRCS))

LOCAL_TARGETS := libbinder.so servicemanager

.PHONY: all clean install
# rules
all: $(LOCAL_TARGETS)

libbinder.so: $(BINDER_OBJS)
	$(CXX) -shared -O3 -g $(LDFLAGS) -o $@ $^

servicemanager: $(SVCMGR_OBJS)
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $^

%.o: %.c
	$(CC) -c $(CFLAGS) $(LDFLAGS) $< -o $@

%.lo: %.cpp
	$(CXX) -c $(CFLAGS) $(LDFLAGS) $< -o $@

clean:
	rm -f *.o $(LOCAL_TARGETS)
	rm -rf $(STAGING_DIR)/usr/include/libbinder
	rm -rf $(STAGING_DIR)/usr/lib/libbinder.so
	rm -rf $(TARGET_DIR)/usr/lib/libbinder.so
	rm -rf $(STAGING_DIR)/usr/bin/servicemanager
	rm -rf $(TARGET_DIR)/usr/bin/servicemanager

install:
	install -m 644 libbinder.so $(STAGING_DIR)/usr/lib
	install -m 644 libbinder.so $(TARGET_DIR)/usr/lib
	$(STRIP) $(TARGET_DIR)/usr/lib/libbinder.so
	mkdir -p $(STAGING_DIR)/usr/include/binder
	mkdir -p $(STAGING_DIR)/usr/include/utils
	install -m 644 $(LOCAL_PATH)/include/binder/*  $(STAGING_DIR)/usr/include/binder
	install -m 644 $(LOCAL_PATH)/include/utils/*  $(STAGING_DIR)/usr/include/utils
	install -m 755 servicemanager $(STAGING_DIR)/usr/bin
	install -m 755 servicemanager $(TARGET_DIR)/usr/bin
	$(STRIP) $(TARGET_DIR)/usr/bin/servicemanager
	install -m 755 $(LOCAL_PATH)/servicemgr/S52_binderSmr $(TARGET_DIR)/etc/init.d/S52_binderSmr
