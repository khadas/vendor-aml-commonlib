UTILS = wifi_power multi_wifi_load_driver input_eventd usbpower usbtestpm usbtestpm_mx usbtestpm_mx_iddq usbpower_mx_iddq usb_monitor
ifeq ($(USE_SIMULATE_KEY),y)
UTILS += simulate_key
endif
ifeq ($(USE_AUTO_SHUTDOWN),y)
UTILS += auto_shutdown
endif
ifeq ($(USE_BATTERY_MONITOR),y)
UTILS += battery_monitor
endif

.PHONY: all install clean
#CFLAGS += $(AML_UTIL_PRIV_FLAGS)

$(info CFLAGS = $(CFLAGS))
wifi_power: wifi_power.c
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $^ $(LDLIBS)

multi_wifi_load_driver: multi_wifi_load_driver.c
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $^ $(LDLIBS)

input_eventd: input_eventd.cpp events_process.cpp events.cpp
	$(CXX) $(CFLAGS) $(LDFLAGS) -o $@ $^ $(LDLIBS) -lpthread

usbpower: usbpower.cpp usbctrl.cpp
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $^ $(LDLIBS)

usbtestpm: usbtestpm.cpp usbctrl.cpp
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $^ $(LDLIBS)

usbtestpm_mx: usbtestpm_mx.cpp usbctrl.cpp
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $^ $(LDLIBS)

usbtestpm_mx_iddq: usbtestpm_mx_iddq.cpp usbctrl_mx_iddq.cpp
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $^ $(LDLIBS)

usbpower_mx_iddq: usbpower_mx_iddq.cpp usbctrl_mx_iddq.cpp
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $^ $(LDLIBS)

usb_monitor: usb_monitor.c
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $^ $(LDLIBS)

simulate_key: simulate_key.c
	$(CC) $(CFLAGS) $(LDFLAGS) -D IR_REMOTE_DEVICE=\"$(IR_REMOTE_DEVICE)\" -o $@ $^ $(LDLIBS)

ioctl: ioctl.c
	$(CC) -g3 -Wall -o $@ $^ -static

auto_shutdown: auto_shutdown.c
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $^ $(LDLIBS) -lpthread

battery_monitor: battery_monitor.c
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $^ $(LDLIBS)

all: $(UTILS)


clean:
	rm -f *.o $(UTILS)

install_ioctl:
	install -m 755 ioctl $(TARGET_DIR)/usr/bin/

install:
	install -m 755 $(UTILS) $(TARGET_DIR)/usr/bin/

uninstall:
	rm -f $(TARGET_DIR)/usr/bin/$(UTILS)
	rm -f $(TARGET_DIR)/usr/bin/ioctl
