#
## sample Makefile for logwrapper
#
#
LOGWRAPPER_DIR ?= .
OBJ = logwrapper.o
CFLAGS = -Wall -I $(LOGWRAPPER_DIR)/include
LDFLAGS = -L$(STAGING_DIR)/usr/lib -llog
TARGET=logwrapper

# rules
all: $(TARGET)

$(TARGET): $(OBJ)
	$(CXX) $(LDFLAGS) $^ -o $@

%.o: %.cpp
	$(CXX) -c $(CFLAGS) $^ -o $@

.PHONY: clean

clean:
	rm -f $(OBJ)

install:
	install -m 755 $(TARGET) $(TARGET_DIR)/usr/bin
