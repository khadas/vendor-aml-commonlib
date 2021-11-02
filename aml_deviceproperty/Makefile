#LIBLOG_SRC_DIR ?= .
CC ?= gcc
LD ?= ld
OBJC = aml_device_property.o
CFLAGS += -fPIC
LDFLAGS +=
LDLIBS = -L./ -lamldeviceproperty
TARGET=libamldeviceproperty.so

# rules
all: $(TARGET)

$(TARGET): $(OBJC)
	$(CC) -shared  -o $@ $^

test_property: test_property.o $(TARGET)
	$(CC) $(LDFLAGS) -o $@ $^ $(LDLIBS)

$(OBJC):%.o:%.c
	$(CC) -c $(CFLAGS) $< -o $@

.PHONY: clean

clean:
	rm -f $(OBJC) $(TARGET)
	rm -f test_property test_property.o

#install:
#	install -m 755 $(TARGET) $(STAGING_DIR)/usr/lib
#	install -m 755 $(TARGET) $(TARGET_DIR)/usr/lib
