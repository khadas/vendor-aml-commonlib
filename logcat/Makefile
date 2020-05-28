#
## sample Makefile for logcat
#
#
LOGCAT_DIR ?= .
OBJ = logcat.o
CFLAGS = -Wall -Wextra -I $(LOGCAT_DIR)/include -DHAVE_IOCTL
LDFLAGS = -L$(STAGING_DIR)/usr/lib -llog
TARGET=logcat
TEST := logcat_test

# rules
all: $(TARGET) $(TEST)

$(TARGET): $(OBJ)
	$(CXX) $(LDFLAGS) $^ -o $@

$(TEST): logcat_test.c
	$(CC) $(LDFLAGS) $^ -o $@

%.o: %.cpp
	$(CXX) -c $(CFLAGS) $^ -o $@

.PHONY: clean

clean:
	rm -f $(OBJ)

install:
	install -m 755 $(TARGET) $(TARGET_DIR)/usr/bin
