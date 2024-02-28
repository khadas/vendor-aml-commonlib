BASEDIR=$(shell basename $(CURDIR))

all:
	@echo " [BUILD] $(BASEDIR)..."
	@mkdir -p $(OBJDIR)/$(BASEDIR)
	@make -f makefile.client all
	@make -f makefile.server all

install:
	@echo " [INSTALL] $(BASEDIR)..."
	@make -f makefile.client install
	@make -f makefile.server install

clean:
	@echo " [CLEAN] $(BASEDIR)..."
	#@make -f makefile.client clean
	@make -f makefile.server clean

.PHONY: all clean install
