BASEDIR=$(shell basename $(CURDIR))

SRCS := $(wildcard *.cpp)
OBJS := $(addprefix $(OBJDIR)/$(BASEDIR)/,$(notdir $(SRCS:.cpp=.o)))


all: $(OBJS)

clean:
	@echo " [CLEAN] $(BASEDIR)..."
	@rm -rf $(OBJDIR)/$(BASEDIR) $(TARGET)

.PHONY: all clean

include ../makefile.inc
