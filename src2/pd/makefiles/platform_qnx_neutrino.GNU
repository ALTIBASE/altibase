# platform_qnx_neutrino.GNU,v 4.9 1999/10/27 14:48:29 levine Exp

# QNX 4.25 hosted GNU g++ for Neutrino 2.0

ifeq (,$(debug))
  debug = 1
endif # ! debug
ifeq (,$(optimize))
  optimize = 1
endif # ! optimize

CC      = qcc
CXX     = QCC
CFLAGS  +=
CCFLAGS += $(CFLAGS)
DCFLAGS += -g
CDLD    = $(CC)
DLD     = $(CXX)
LD      = $(CXX)
LDFLAGS +=
LIBS    += -lsocket -lstdc++
OCFLAGS += -O2
PIC     = -shared
AR      = ar
ARFLAGS = -ruv
RANLIB  = ranlib
SOFLAGS = -G -shared
SOBUILD = $(COMPILE.cc) $(PIC) -o $(VSHDIR)$*.so $<
PRELIB  = @true

