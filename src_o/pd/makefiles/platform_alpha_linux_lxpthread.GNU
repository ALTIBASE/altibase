# platform_linux_lxpthread.GNU,v 4.38 2000/02/02 16:44:27 coryan Exp

# For pthreads support on Linux, you need
# -D_POSIX_THREADS
# -D_POSIX_THREAD_SAFE_FUNCTIONS
# in the CXX command line. Also, add -lpthreads to the LIBS.
# libpthreads.so comes with the sources of Linux libc-5.3.*, you need
# to compile it yourself (no binaries included) --
# Jan Rychter <jwr@icm.edu.pl>

ifeq (,$(debug))
  debug = 1
endif
ifeq (,$(optimize))
  optimize = 1
endif

PLATFORM_AIO_SUPPORT := \
  $(shell test -e /usr/lib/librt.so && echo -DPDL_HAS_AIO_CALLS)

ifeq ($(NO_IMPLICIT_TEMPLATES),)
      NO_IMPLICIT_TEMPLATES = -fno-implicit-templates
endif

CC      = gcc
CXX     = g++
CFLAGS  += -W -Wall -pipe \
           -D_POSIX_PTHREAD_SEMANTICS -D_POSIX_THREADS -D_POSIX_THREAD_SAFE_FUNCTIONS -D_REENTRANT \
           $(PLATFORM_AIO_SUPPORT)
CCFLAGS += $(CFLAGS) $(NO_IMPLICIT_TEMPLATES) 
DCFLAGS += -g -DDEBUG
CDLD    = $(CC)
DLD     = $(CXX)
LD      = $(CXX)
LIBS    += -ldl -lpthread
ifeq (-DPDL_HAS_AIO_CALLS,$(PLATFORM_AIO_SUPPORT))
  LIBS += -lrt
endif
OCFLAGS += -O3 -mcpu=ev6 -funroll-all-loops -mieee
PCFLAGS  = -pg
PCCFLAGS = -pg
PIC     = -fPIC
AR      = ar
ARFLAGS = rsuv
RANLIB  = @true
SOFLAGS = $(CPPFLAGS) -shared
SOBUILD = $(COMPILE.cc) $(PIC) -o $(VSHDIR)$*.o $<; \
          $(SOLINK.cc) -o $@ $(LDFLAGS) $(VSHDIR)$*.o
# Added line below to support "Executable Shared Object" files (as
# needed by the service configurator).
# Marius Kjeldahl <mariusk@sn.no, marius@funcom.com>
ESOBUILD = $(COMPILEESO.cc) $(PIC) -shared -o $(VSHDIR)$*.so $<
ifndef PRELIB
  PRELIB = @true
endif # ! PRELIB


#### GNU gas has a string limit of 4096 characters.  On Alphas,
#### builds will fail due to running over that limit.  There are
#### at least two workarounds:
#### 1) Change the limit to 8192 characters and rebuild gas.  See
####    PDL-INSTALL.html for more information.
#### 2) Don't use -g when compiling those files.
#### If you're building on an Alpha and you haven't hacked and
#### rebuilt gas, you might need to uncomment the following.
#### ifeq ($(debug),1)
####   SUPPRESS_DASH_G = 1
#### endif # debug
