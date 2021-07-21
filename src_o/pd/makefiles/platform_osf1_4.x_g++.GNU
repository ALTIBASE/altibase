# platform_osf1_4.x_g++.GNU,v 4.3 2002/04/22 02:01:22 kitty Exp

# This platform macros file is intended to work with Digital UNIX 4.x
# (OSF/1 4.x) and GCC in version 2.7.2.1 (or newer).

ifeq (,$(debug))
  debug = 1
endif

ifeq (,$(exceptions))
  exceptions = 1
endif
ifeq (,$(rtti))
   rtti = 1
endif

ifeq (,$(threads))
  threads = 1
endif

ifeq (,$(optimize))
  optimize = 1
endif

CC              = gcc
CXX             = g++
CFLAGS          += -D_REENTRANT

# Test for template instantiation.
#
include $(PD_DIR)/makefiles/platform_g++_common.GNU
ifeq (3.,$(findstring 3.,$(CXX_VERSION)))
 use_strict_prototype=no
else
 use_strict_prototype=yes
endif

CCFLAGS         += $(CFLAGS) $(TEMPLATES_FLAG)

ifeq ($(use_strict_prototype),yes)
  CCFLAGS  += -fno-strict-prototypes
endif


DCFLAGS         += -g
LD              = $(CXX)
CDLD            = $(CC)
DLD             = $(CXX)
LIBS            += -lpthread -lmach -lexc -ltli -lrt
OCFLAGS         += -O0
PIC             = -fpic
ARFLAGS         = cruvZ
# When libraries are archived, a hash index is automatically created so there
# is no need for ranlib
RANLIB          = /usr/bin/true
SOFLAGS         = -shared $(CPPFLAGS) $(PDLLIB)
SOBUILD         = $(COMPILE.cc) $(PIC) -o $(VSHDIR)$*.so $<
PRELIB          = @true
