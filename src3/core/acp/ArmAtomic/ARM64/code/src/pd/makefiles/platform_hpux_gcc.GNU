# platform_hpux_gcc.GNU,v 4.20 2000/02/04 20:59:23 shuston Exp
#

ifeq (,$(debug))
  debug = 1
endif
ifeq (,$(optimize))
  optimize = 0
endif
ifeq (,$(threads))
  threads = 1
endif

# First, extract the OS version number.
HPVERS_WORDS := $(subst ., ,$(shell uname -r))
HPUX_VERS	:= $(join $(word 2,$(HPVERS_WORDS)),$(word 3,$(HPVERS_WORDS)))
#
CC		= gcc
CXX		= g++
CFLAGS		+= -w
CXX_VERSION := $(shell $(CXX) -dumpversion)
ifeq (3.,$(findstring 3.,$(CXX_VERSION)))
 use_strict_prototype=no
else
 use_strict_prototype=yes
endif

#
# Set the appropriate preprocessor defs for threading based on OS version.
# If the user has requested building without threads, then don't set any.
#
ifeq ($(threads),1)
ifeq ($(word 2,$(HPVERS_WORDS)), 11)
THR_DEFS	= -DPDL_HAS_THREADS -D_POSIX_C_SOURCE=199506L
else
THR_DEFS	= -DPDL_HAS_THREADS -D_REENTRANT
endif
endif

CCFLAGS		+= $(CFLAGS) -fno-implicit-templates -DHPUX_VERS=$(HPUX_VERS) $(THR_DEFS)

GCC_332         = 3.3.2
ifeq ($(CXX_VERSION), $(GCC_332))
CCFLAGS         += -DHP_GCC_332 -D_INCLUDE_LONGLONG
endif
GCC_402         = 4.0.2
ifeq ($(CXX_VERSION), $(GCC_402))
CCFLAGS         += -DHP_GCC_402
endif

ifeq ($(compile64),1)
CCFLAGS			+= -mlp64
CFLAGS			+= -mlp64
endif

# On HP-UX 11, there's a conflict with pthread_atfork in sys/unistd.h and
# sys/pthread.h - both define the function, but slightly differently.
# -fstrict-prototype resolves the issue.
ifeq ($(word 2,$(HPVERS_WORDS)), 11)

ifeq ($(use_strict_prototype),yes)
  CCFLAGS  += -fstrict-prototype
endif

endif
DCFLAGS		+= -g
CDLD		= $(CC)
DLD		= $(CXX)
LD		= $(CXX)
OCFLAGS		+= -O2
PIC		= -fPIC
ARFLAGS		= ruv
RANLIB		= echo
LDFLAGS		= -L. -L$(PD_DIR)/pdl -Wl,+s
SOFLAGS		= $(CPPFLAGS) -shared -nostdlib $(PIC)
SOEXT		= sl
SOBUILD 	= $(COMPILE.cc) $(PIC) -o $(VSHDIR)$*.$(SOEXT) $<
SHLIBBUILD 	= $(SOLINK.cc) -o $@ $(VSHOBJS1)
PRELIB = @true
#
# Grab the appropriate libraries based on the version of HP-UX we're building
# on and whether or not thread support is being used.
#
ifeq ($(word 2,$(HPVERS_WORDS)), 11)

#11.x:
ifeq ($(threads),1)
LIBS		+= -lxti -lpthread -lrt -ldld
else
LIBS		+= -lxti -lrt -ldld
endif

else

# 10.x:
LIBS		+= -lxti -ldld
ifeq ($(threads),1)
LIBS		+= -lcma
endif

endif
