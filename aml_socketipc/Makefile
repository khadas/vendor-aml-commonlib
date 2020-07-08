CFLAGS += -Wall -g
SRC = aml_socketipc.c
TARGET = libaml_socketipc.so

all: $(TARGET)

$(TARGET): $(SRC)
	$(CC) $(CFLAGS) $(LDFLAGS) -shared -fPIC \
	-o $@ $^ $(LDLIBS)

.PHONY: clean

clean:
	rm $(TARGET)

install:
	install -m 0644 libaml_socketipc.so $(TARGET_DIR)/usr/lib/
	install -m 0644 aml_socketipc.h $(STAGING_DIR)/usr/include/
	install -m 0644 libaml_socketipc.so $(STAGING_DIR)/usr/lib/

uninstall:
	rm -f $(TARGET_DIR)/usr/lib/libaml_socketipc.so
