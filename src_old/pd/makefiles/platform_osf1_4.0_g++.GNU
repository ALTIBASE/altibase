# platform_osf1_4.0_g++.GNU,v 4.18 1998/01/27 03:16:12 levine Exp

# This platform macros file is intended to work with Digital UNIX 4.0
# (OSF/1 4.0) and GCC in version 2.7.2.1 (or newer).

debug = 1
optimize = 1

CC              = gcc
CXX             = g++

CXX_VERSION := $(shell $(CXX) -dumpversion)

ifeq (3.,$(findstring 3.,$(CXX_VERSION)))
 use_strict_prototype=no
else
 use_strict_prototype=yes
endif

CFLAGS          += -D_REENTRANT
CCFLAGS         += $(CFLAGS) -fno-implicit-templates

ifeq ($(use_strict_prototype),yes)
  CCFLAGS  +=  -fno-strict-prototypes
endif

DCFLAGS         += -g
LD              = $(CXX)
DLD             = $(CXX)
LIBS            += -lpthread -lmach -lexc -ltli -lrt
OCFLAGS         += -O0
PIC             = -fpic
ARFLAGS         = cruvZ
# When libraries are archived, a hash index is automatically created so there
# is no need for ranlib
RANLIB          = /usr/bin/true
SOFLAGS         = -shared $(CPPFLAGS)
SOBUILD         = $(COMPILE.cc) $(PIC) -o $(VSHDIR)$*.so $<
PRELIB          = @true
