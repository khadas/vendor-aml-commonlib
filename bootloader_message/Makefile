LIB = libbootloader_message.a
.PHONY: all install clean

#bootloader_message.o: bootloader_message.c
#	$(CC) -c bootloader_message.c

all: urlmisc bootloader_message.o test_ab
	$(AR) rc $(LIB) bootloader_message.o

urlmisc: urlmisc.o bootloader_message.o
	$(CC) $(CFLAGS) $(LDFLAGS) $^ -o $@

test_ab : test.o bootloader_message.o
	$(CC) $(CFLAGS) $(LDFLAGS) $^ -o $@


clean:
	rm -f *.o $(LIB)

install:
	install -m 755 $(LIB) $(STAGING_DIR)/usr/lib
	install -m 755 urlmisc $(TARGET_DIR)/usr/bin

