# platform_sco-nothread.GNU,v 4.16 1997/09/22 00:37:57 levine Exp

debug = 1
optimize = 1

CC              = gcc
CXX		= g++

CXX_VERSION := $(shell $(CXX) -dumpversion)

ifeq (3.,$(findstring 3.,$(CXX_VERSION)))
 use_strict_prototype=no
else
 use_strict_prototype=yes
endif

CFLAGS		+= -w
CCFLAGS		+= $(CFLAGS) -fno-implicit-templates

ifeq ($(use_strict_prototype),yes)
  CCFLAGS  += -fno-strict-prototypes
endif

DCFLAGS		+= -g #### should this be -gstabs?
CDLD            = $(CC)
DLD             = $(CXX)
LD		= $(CXX)
LIBS            = -lsocket
OCFLAGS		= -O2
PIC             =
AR              = ar
ARFLAGS         = ruv
RANLIB          = /bin/true

SOFLAGS = $(CPPFLAGS)
SOBUILD = $(COMPILE.cc) $(PIC) -o $(VSHDIR)$*.so $<
PRELIB  = @true
