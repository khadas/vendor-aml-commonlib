LIB = libubootenv.so
INCLUDE = ubootenv.h
OUT_DIR ?= .

CFLAGS += -fPIC
LDFLAGS := -Wl,-soname,$(LIB) -shared -pthread

.PHONY: all install clean

ubootenv.o: ubootenv.c
	$(CC) $(CFLAGS) -c $< -o $(OUT_DIR)/$@

uenv_test.o: uenv_test.c
	$(CC) $(CFLAGS) -c $< -o $(OUT_DIR)/$@

all: $(LIB) uenv

$(LIB): ubootenv.o
	$(CC) $(LDFLAGS) $(patsubst %.o, $(OUT_DIR)/%.o, $<) -o $(OUT_DIR)/$@

uenv: uenv_test.o $(LIB)
	$(CC) -L$(OUT_DIR) -lubootenv $(patsubst %.o, $(OUT_DIR)/%.o, $<) -o $(OUT_DIR)/$@

clean:
	rm -f $(OUT_DIR)/*.o $(OUT_DIR)/$(LIB)
	rm -f $(OUT_DIR)/uenv

install:
	install -m 755 $(OUT_DIR)/$(LIB) $(TARGET_DIR)/usr/lib
	install -m 755 $(OUT_DIR)/$(LIB) $(STAGING_DIR)/usr/lib
	install -m 755 $(OUT_DIR)/uenv $(TARGET_DIR)/usr/bin
	install -m 644 $(INCLUDE) $(STAGING_DIR)/usr/include
