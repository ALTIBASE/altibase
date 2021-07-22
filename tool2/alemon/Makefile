# Copyright 1999-2007, ALTIBASE Corporation or its subsidiaries.
# All rights reserved.

# $Id: Makefile 845 2007-09-19 07:13:58Z shsuh $
#


include ../../makefiles/root.mk
include $(MAKEFILES_DIR)/rule.mk

TARGET = $(ALTI_HOME)/bin/alemon$(EXEC_SUF)
SOURCE = lemon.c

all: $(TARGET)

$(TARGET): $(SOURCE)
ifeq ($(ALTI_CFG_OS),WINDOWS)
	cl $< /Fe$@
else
	gcc -g -o $@ $<
endif

install: $(TARGET)

clean:
	rm -f $(TARGET)

.PHONY: all install clean
