CFLAGS += -Wall -g -I../aml_log
LDFLAGS += -L../aml_log
LDLIBS += -laml_log -lwpa_client -lpthread

SRC = aml_wpa_wifi_util.c
TARGET = libaml_wpa_wifi_util.so
BIN_TARGET = aml_wpa_wifi_test_demo

# determine if you need to build the demo as needed, set 1 to build, set 0 not to build
BUILD_BIN_TARGET := 0

ifeq ($(BUILD_BIN_TARGET),1)
TARGET += $(BIN_TARGET)
endif

all: $(TARGET)

$(TARGET): $(SRC)
	$(CC) $(CFLAGS) $(LDFLAGS) -shared -fPIC \
	-o $@ $^ $(LDLIBS)

ifeq ($(BUILD_BIN_TARGET),1)
	$(CC) $(CFLAGS) -o $(BIN_TARGET) aml_wpa_wifi_test_demo.c -L. -laml_wpa_wifi_util $(LDLIBS) -w
endif

clean:
	rm $(TARGET)

install:
	install -m 0644 libaml_wpa_wifi_util.so $(TARGET_DIR)/usr/lib/
	if [ -f "$(BIN_TARGET)" ]; then install -m 0755 $(BIN_TARGET) $(TARGET_DIR)/usr/bin/; fi
	install -m 0644 aml_wpa_wifi_util.h $(STAGING_DIR)/usr/include/
	install -m 0644 libaml_wpa_wifi_util.so $(STAGING_DIR)/usr/lib/

uninstall:
	rm -f $(TARGET_DIR)/usr/lib/libaml_wpa_wifi_util.so
	if [ -f "$(TARGET_DIR)/usr/bin/$(BIN_TARGET)" ]; then rm -f $(TARGET_DIR)/usr/bin/$(BIN_TARGET); fi