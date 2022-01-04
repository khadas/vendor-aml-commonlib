LIB = libubootenv.a
INCLUDE = ubootenv.h
OUT_DIR ?= .
.PHONY: all install clean

ubootenv.o: ubootenv.c
	$(CC)  -fPIC -c ubootenv.c  -o $(OUT_DIR)/$@

uenv_test.o: uenv_test.c
	$(CC) -c uenv_test.c -o $(OUT_DIR)/$@

all: ubootenv.o uenv
	$(AR) rc $(OUT_DIR)/$(LIB) $(OUT_DIR)/ubootenv.o

uenv: ubootenv.o uenv_test.o
	$(CC) $(patsubst %.o,$(OUT_DIR)/%.o,$^) -lz -fPIC -o $(OUT_DIR)/$@

clean:
	rm -f $(OUT_DIR)/*.o $(OUT_DIR)/$(LIB)

install:
	install -m 755 $(OUT_DIR)/$(LIB) $(STAGING_DIR)/usr/lib
	install -m 755 $(OUT_DIR)/uenv $(TARGET_DIR)/usr/bin
	install -m 644 $(INCLUDE) $(STAGING_DIR)/usr/include
