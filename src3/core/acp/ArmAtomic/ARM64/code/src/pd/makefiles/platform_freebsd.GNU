# platform_freebsd.GNU,v 4.11 1999/06/21 18:24:30 levine Exp

# platform_freebsd.GNU

VERSION := $(shell uname -r | sed -e "s/`uname -r | sed -e 's/.//'`//")
ifneq ($(VERSION),3)
versioned_so = 1
endif

optimize = 1

debug = 1

CC              = gcc
CXX             = g++
CFLAGS          += -W -Wall -Wpointer-arith -pipe
CCFLAGS         += $(CFLAGS) -fno-implicit-templates
DCFLAGS         += -g
ifeq ($(VERSION),3)
  LDFLAGS       += -Wl,-rpath $(PD_DIR)/pdl
endif
CDLD            = $(CC)
DLD             = $(CXX)
LD              = $(CXX)
LIBS            += -lstdc++
OCFLAGS         += -O2
PIC             = -fpic
AR              = ar
ARFLAGS         = ruv
RANLIB          = ranlib

SOFLAGS = $(CPPFLAGS) -shared
SOBUILD = $(COMPILE.cc) $(PIC) -o $(VSHDIR)$*.so $<
PRELIB = @echo
