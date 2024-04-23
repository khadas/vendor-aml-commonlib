OUT_DIR ?= .
UTILS = wifi_power multi_wifi_load_driver input_eventd usbpower usbtestpm usbtestpm_mx usbtestpm_mx_iddq usbpower_mx_iddq usb_monitor vad_service
ifeq ($(USE_SIMULATE_KEY),y)
UTILS += simulate_key
endif
ifeq ($(USE_AUTO_SHUTDOWN),y)
UTILS += auto_shutdown
endif
ifeq ($(USE_BATTERY_MONITOR),y)
UTILS += battery_monitor
endif
ifeq ($(USE_UAC_HID),y)
UTILS += uac_hid
endif

.PHONY: all install clean
#CFLAGS += $(AML_UTIL_PRIV_FLAGS)

$(info OUT_DIR = $(OUT_DIR))
$(info CFLAGS = $(CFLAGS))
wifi_power: wifi_power.c
	$(CC) $(CFLAGS) $(LDFLAGS) -o $(OUT_DIR)/$@ $^ $(LDLIBS)

multi_wifi_load_driver: multi_wifi_load_driver.c
	$(CC) $(CFLAGS) $(LDFLAGS) -o $(OUT_DIR)/$@ $^ $(LDLIBS)

input_eventd: input_eventd.cpp events_process.cpp events.cpp
	$(CXX) $(CFLAGS) $(LDFLAGS) -o $(OUT_DIR)/$@ $^ $(LDLIBS) -lpthread

usbpower: usbpower.cpp usbctrl.cpp
	$(CC) $(CFLAGS) $(LDFLAGS) -o $(OUT_DIR)/$@ $^ $(LDLIBS)

usbtestpm: usbtestpm.cpp usbctrl.cpp
	$(CC) $(CFLAGS) $(LDFLAGS) -o $(OUT_DIR)/$@ $^ $(LDLIBS)

usbtestpm_mx: usbtestpm_mx.cpp usbctrl.cpp
	$(CC) $(CFLAGS) $(LDFLAGS) -o $(OUT_DIR)/$@ $^ $(LDLIBS)

usbtestpm_mx_iddq: usbtestpm_mx_iddq.cpp usbctrl_mx_iddq.cpp
	$(CC) $(CFLAGS) $(LDFLAGS) -o $(OUT_DIR)/$@ $^ $(LDLIBS)

usbpower_mx_iddq: usbpower_mx_iddq.cpp usbctrl_mx_iddq.cpp
	$(CC) $(CFLAGS) $(LDFLAGS) -o $(OUT_DIR)/$@ $^ $(LDLIBS)

usb_monitor: usb_monitor.c
	$(CC) $(CFLAGS) $(LDFLAGS) -o $(OUT_DIR)/$@ $^ $(LDLIBS)

simulate_key: simulate_key.c
	$(CC) $(CFLAGS) $(LDFLAGS) -D IR_REMOTE_DEVICE=\"$(IR_REMOTE_DEVICE)\" -o $(OUT_DIR)/$@ $^ $(LDLIBS)

ioctl: ioctl.c
	$(CC) -g3 -Wall -o $(OUT_DIR)/$@ $^ -static

auto_shutdown: auto_shutdown.c
	$(CC) $(CFLAGS) $(LDFLAGS) -o $(OUT_DIR)/$@ $^ $(LDLIBS) -lpthread

battery_monitor: battery_monitor.c
	$(CC) $(CFLAGS) $(LDFLAGS) -o $(OUT_DIR)/$@ $^ $(LDLIBS)

uac_hid: uac_hid.c
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $^ $(LDLIBS)

fota_upgrade: fota_upgrade.c ../aml_utility/unifykey.c
	$(CC) $(CFLAGS) $(LDFLAGS) -o $(OUT_DIR)/$@ $^ $(LDLIBS) -L../fota -ldmgr -lamldeviceproperty -lm -ldl -lpthread

vad_service: vad_tool/aml_alsa_mixer.c vad_tool/alsa_device_parser.c vad_tool/aml_vad_wakeup.c vad_tool/vad_service.c vad_tool/tinyalsa/mixer.c vad_tool/tinyalsa/pcm.c
	$(CC) $(CFLAGS) $(LDFLAGS) -I./vad_tool  -I./vad_tool/tinyalsa/include/tinyalsa  -o $@ $^ $(LDLIBS) -lpthread

all: $(UTILS)

UTILS_FULLPATH := $(patsubst %, $(OUT_DIR)/%, $(UTILS))
$(info UTILS_FULLPATH = $(UTILS_FULLPATH))

clean:
	rm -f $(OUT_DIR)/*.o $(UTILS_FULLPATH)

install_ioctl:
	install -m 755 $(OUT_DIR)/ioctl $(TARGET_DIR)/usr/bin/

install:
	install -m 755 $(UTILS_FULLPATH) $(TARGET_DIR)/usr/bin/

uninstall:
	rm -f $(TARGET_DIR)/usr/bin/$(UTILS)
	rm -f $(TARGET_DIR)/usr/bin/ioctl
