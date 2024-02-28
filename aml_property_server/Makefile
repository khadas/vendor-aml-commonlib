BASEDIR=$(shell basename $(CURDIR))
BUILDDIR ?= $(CURDIR)
OBJDIR = $(BUILDDIR)/objs
LIB_VER := 0

export BUILDDIR
export OBJDIR
export LIB_VER
export CPPFLAGS

all:
	@echo " [BUILD] $(BASEDIR)..."
	@make -C $(CURDIR)/common all
	@make -C $(CURDIR)/property-db all
	@make -C $(CURDIR)/dbus all
	@make -C $(CURDIR)/tools all

install:
	@echo " [INSTALL] $(BASEDIR)..."
	@make -C $(CURDIR)/dbus install
	@make -C $(CURDIR)/tools install

clean:
	@echo " [CLEAN] $(BASEDIR)..."
	@make -C $(CURDIR)/property-db clean
	@make -C $(CURDIR)/dbus clean
	@make -C $(CURDIR)/tools clean

.PHONY: all clean install
