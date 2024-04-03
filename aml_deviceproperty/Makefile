OUT_DIR ?= .
#LIBLOG_SRC_DIR ?= .
CC ?= gcc
LD ?= ld
OBJC = aml_device_property.o
CFLAGS += -fPIC
LDFLAGS += -L $(OUT_DIR)/
LDLIBS = -L./ -lamldeviceproperty
TARGET=libamldeviceproperty.so

$(info "OUT_DIR : $(OUT_DIR)")

# rules
all: $(TARGET)

$(TARGET): $(OBJC)
	$(CC) -shared $(patsubst %, $(OUT_DIR)/%, $^) -o $(OUT_DIR)/$@

test_property: test_property.o $(TARGET)
	$(CC) $(LDFLAGS) $(patsubst %, $(OUT_DIR)/%, $^) -o $(OUT_DIR)/$@ $(LDLIBS)

%.o:%.c
	$(CC) -c $(CFLAGS) $< -o $(OUT_DIR)/$@

.PHONY: clean

clean:
	rm -f $(OUT_DIR)/*.o $(OUT_DIR)/$(TARGET)
	rm -f $(OUT_DIR)/test_property

#install:
#	install -m 755 $(TARGET) $(STAGING_DIR)/usr/lib
#	install -m 755 $(TARGET) $(TARGET_DIR)/usr/lib
