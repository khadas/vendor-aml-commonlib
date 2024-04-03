#
## sample Makefile for logcat
#
#
OUT_DIR ?= .
LOGCAT_DIR ?= .
OBJ = logcat.o
CFLAGS = -Wall -Wextra -I $(LOGCAT_DIR)/include -DHAVE_IOCTL
LDFLAGS = -L$(STAGING_DIR)/usr/lib -llog
TARGET=logcat
TEST := logcat_test

$(info "OUT_DIR : $(OUT_DIR)")
$(info "TARGET_DIR : $(TARGET_DIR)")
$(info "STAGING_DIR : $(STAGING_DIR)")
$(info "DESTDIR : $(DESTDIR)")
$(info "LOGCAT_DIR : $(LOGCAT_DIR)")

# rules
all: $(TARGET) $(TEST)

$(TARGET): $(OBJ)
	$(CXX) $(LDFLAGS)  $(OUT_DIR)/$^ -o $(OUT_DIR)/$@

$(TEST): logcat_test.c
	$(CC) $(LDFLAGS) $^ -o $(OUT_DIR)/$@

%.o: %.cpp
	$(CXX) -c $(CFLAGS) $^ -o $(OUT_DIR)/$@

.PHONY: clean

clean:
	rm -f $(OUT_DIR)/$(OBJ)

install:
	install -m 755 $(OUT_DIR)/$(TARGET) $(TARGET_DIR)/usr/bin
