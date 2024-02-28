BASEDIR=$(shell basename $(CURDIR))

SRCS := $(wildcard *.cpp)

OBJS := $(addprefix $(OBJDIR)/$(BASEDIR)/,$(notdir $(SRCS:.cpp=.o)))

CPPFLAGS += -Wno-unused-function

LDFLAGS += -Wl,-rpath,.
LD_LIBS := -L$(BUILDDIR) -laml-property


TARGET_NAME := prop
TARGET := $(BUILDDIR)/$(TARGET_NAME)

$(TARGET): $(OBJS)
	@echo " [LINK] $@"
	@$(CXX) -o $@ $^ $(LDFLAGS) $(LD_LIBS)

install: $(TARGET)
	@echo " [INSTALL] $(TARGET_NAME)..."
	@install -d ${DESTDIR}/usr/bin
	@install -m 0755 $(TARGET) $(DESTDIR)/usr/bin/

clean:
	@echo " [CLEAN] $(BASEDIR)..."
	@rm -rf $(OBJDIR)/$(BASEDIR) $(TARGET)

all: $(TARGET)

.PHONY: clean install

include ../makefile.inc
