SO_VERSION = 1
SO_REALNAME = 1.0.0
LIBLOG_SRC_DIR ?= .
OBJC = logd_write.o logprint.o event_tag_map.o fake_log_device.o
CFLAGS += -fPIC -I$(LIBLOG_SRC_DIR)/include/
LDFLAGS += -L./lib -lglibc_bridge -lc++.so -lc++abi.so
TARGET=liblog.so
SONAME_TARGET=liblog.so.${SO_VERSION}
REAL_TARGET=liblog.so.${SO_REALNAME}

# rules
all: $(TARGET) ${SONAME_TARGET} ${REAL_TARGET}

$(REAL_TARGET): $(OBJC)
	$(CC) -fPIC -g -shared -Wl,-soname,${SONAME_TARGET} -o $@ $^

$(SONAME_TARGET): $(REAL_TARGET)
	ln -s $^ $@

$(TARGET): $(SONAME_TARGET)
	ln -s $^ $@

$(OBJC):%.o:%.c
	$(CC) -c $(CFLAGS) $< -o $@

.PHONY: clean

clean:
	rm -f $(OBJC) $(TARGET)

install:
	install -m 755 $(TARGET) $(STAGING_DIR)/usr/lib
	install -m 755 $(TARGET) $(TARGET_DIR)/usr/lib
	install -m 755 $(SONAME_TARGET) $(STAGING_DIR)/usr/lib
	install -m 755 $(SONAME_TARGET) $(TARGET_DIR)/usr/lib
	install -m 755 $(REAL_TARGET) $(STAGING_DIR)/usr/lib
	install -m 755 $(REAL_TARGET) $(TARGET_DIR)/usr/lib
	mkdir -p $(STAGING_DIR)/usr/include/android
	mkdir -p $(STAGING_DIR)/usr/include/cutils
	install -m 755 $(LIBLOG_SRC_DIR)/include/android/*  $(STAGING_DIR)/usr/include/android/
	install -m 755 $(LIBLOG_SRC_DIR)/include/cutils/*  $(STAGING_DIR)/usr/include/cutils/
