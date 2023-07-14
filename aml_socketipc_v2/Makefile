CFLAGS += -Wall -g -I../aml_log
LDFLAGS += -L../aml_log
LDLIBS += -laml_log

SRC_DIR = common
SRC = $(SRC_DIR)/util_list.c $(SRC_DIR)/socket_client.c $(SRC_DIR)/socket_server.c $(SRC_DIR)/socket_util.c
TARGET = libaml_socketipc_v2.so

all: $(TARGET)

$(TARGET): $(SRC)
	$(CC) $(CFLAGS) $(LDFLAGS) -shared -fPIC \
	-o $@ $^ $(LDLIBS)

clean:
	rm $(TARGET)

install:
	install -m 0644 libaml_socketipc_v2.so $(TARGET_DIR)/usr/lib/
	install -m 0644 common/*.h $(STAGING_DIR)/usr/include/
	install -m 0644 libaml_socketipc_v2.so $(STAGING_DIR)/usr/lib/

uninstall:
	rm -f $(TARGET_DIR)/usr/lib/libaml_socketipc_v2.so