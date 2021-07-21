# platform_sunos4_g++.GNU,v 4.16 1997/09/22 00:37:59 levine Exp

# SunOS 4.x (Solaris 1.x) with g++ *not* using Orbix

debug = 1
optimize = 1

CC	= gcc 
CXX	= g++

CXX_VERSION := $(shell $(CXX) -dumpversion)

ifeq (3.,$(findstring 3.,$(CXX_VERSION)))
 use_strict_prototype=no
else
 use_strict_prototype=yes
endif

ifeq ($(use_strict_prototype),yes)
 CFLAGS  += -fno-strict-prototypes
endif

CCFLAGS += $(CFLAGS) -fno-implicit-templates
DCFLAGS	+= -g
DLD	= /bin/ld
LD	= $(CXX)
#
# for G++ v2.6.0
OCFLAGS	+= -O2 $(added_optimizer_flag)
PIC	= -fpic
AR	= ar
ARFLAGS	= ruv
RANLIB	= ranlib
SOFLAGS	= -assert pure-text 
# SOBUILD - compile into .so directly
SOBUILD = $(COMPILE.cc) $(PIC) -o $(VSHDIR)$*.so $<
# SOLINK	= $(SOLINK.cc) -o $@ $(LDFLAGS) $<
PRELIB	= @true
