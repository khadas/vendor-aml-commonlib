LIB = libbootloader_message.a
OBJS = bootloader_message.o bootloader_avb.o avb_crc32.o
.PHONY: all install clean

#bootloader_message.o: bootloader_message.c
#	$(CC) -c bootloader_message.c

all: urlmisc bootloader_message.o test_ab
	$(AR) rc $(LIB) $(OBJS)

urlmisc: urlmisc.o $(OBJS)
	$(CC) $(CFLAGS) $(LDFLAGS) $^ -o $@

test_ab : test.o $(OBJS)
	$(CC) $(CFLAGS) $(LDFLAGS) $^ -o $@


clean:
	rm -f *.o $(LIB)

install:
	install -m 755 $(LIB) $(STAGING_DIR)/usr/lib
	install -m 755 urlmisc $(TARGET_DIR)/usr/bin

