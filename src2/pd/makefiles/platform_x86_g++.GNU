# platform_sunos5_g++.GNU,v 4.39 1999/06/21 18:24:31 levine Exp

# SunOS 5.x (Solaris 2.x) with g++ *not* using Orbix

# NOTE: with g++ 2.8.0, you'll need to disable optimization in order to
#       instantiate PDL_Map_Manager (pdl/Map_Manager.cpp).  The easiest
#       way to do that is to set "optimize = 0", either below or on
#       your make command line.

# NOTE: On Solaris86, you'll need to use GNU as instead of /usr/ccs/bin/as,
#       if you want -gstabs+ and -pipe support.

ifeq (,$(debug))
  debug = 1
endif
ifeq (,$(optimize))
  optimize = 1
endif
ifeq (,$(threads))
  threads = 1
endif

ifeq ($(threads),0)
  CFLAGS += -DPDL_MT_SAFE=0
endif # threads

# modified by hjohn
buildbits = 32
ifeq ($(compile64),1)
  buildbits = 64
else
  buildbits = 32
endif

ifeq ($(shell /bin/uname -m),i86pc)
  #### gcc on Solaris86 doesn't use -g
  DCFLAGS         += -gstabs+ -DDEBUG
else  # ! i86pc
  DCFLAGS         += -g -DDEBUG
endif # ! i86pc

## added by hjohn
ifeq ($(buildbits),64)
CFLAGS         += -m64
SOFLAGS        += -m64
endif

CC              = gcc
CXX             = g++
CFLAGS          += -W -Wall -Wpointer-arith -pipe -D_POSIX_PTHREAD_SEMANTICS -D_REENTRANT #### -Winline
CCFLAGS         += $(CFLAGS) -fno-implicit-templates
CDLD            = $(CC)
DLD             = $(CXX)
LD              = $(CXX)
LDFLAGS         +=
LIBS            += -lsocket -ldl -lgen -lnsl -lposix4 -lthread
OCFLAGS         += -O3 -funroll-loops $(added_optimizer_flag)
PIC             = -fPIC
AR              = ar
ARFLAGS         = ruv
RANLIB          = @true
SOFLAGS         += -shared $(CPPFLAGS)
SOBUILD         = $(COMPILE.cc) $(PIC) -o $(VSHDIR)$*.so $<
PRELIB          = @true
