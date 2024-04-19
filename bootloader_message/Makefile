OUT_DIR ?= .
LDFLAGS += -L $(OUT_DIR)/
LIB = libbootloader_message.a
OBJS = bootloader_message.o bootloader_avb.o avb_crc32.o
.PHONY: all install clean

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $(OUT_DIR)/$@

all: $(LIB)  urlmisc bootloader_slot

$(LIB): $(OBJS)
	$(AR) rc $(OUT_DIR)/$(LIB) $(patsubst %.o, $(OUT_DIR)/%.o, $(OBJS))

urlmisc: urlmisc.o $(OBJS)
	$(CC) $(CFLAGS) $(LDFLAGS) -L$(OUT_DIR) $(patsubst %.o, $(OUT_DIR)/%.o, $^) -o $(OUT_DIR)/$@

bootloader_slot : test.o $(OBJS)
	$(CC) $(CFLAGS) $(LDFLAGS) -L$(OUT_DIR) $(patsubst %.o, $(OUT_DIR)/%.o, $^) -o $(OUT_DIR)/$@

clean:
	rm -f $(OUT_DIR)/*.o $(OUT_DIR)/$(LIB)

install:
	install -m 755 $(OUT_DIR)/$(LIB) $(STAGING_DIR)/usr/lib
	install -m 755 $(OUT_DIR)/urlmisc $(TARGET_DIR)/usr/bin
