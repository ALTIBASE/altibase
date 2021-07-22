# platform_linux_pthread.GNU,v 4.19 1998/05/04 13:54:25 levine Exp

# For pthreads support on Linux, you need
# -D_MIT_POSIX_THREADS
# -D_POSIX_THREADS
# -D_POSIX_THREAD_SAFE_FUNCTIONS
# in the CXX command line. Also, add -lpthreads to the LIBS.
# libpthreads.so comes with the sources of Linux libc-5.3.*, you need
# to compile it yourself (no binaries included) --
# Jan Rychter <jwr@icm.edu.pl>

debug = 1
optimize = 1

CC      = gcc
CXX     = g++

CXX_VERSION := $(shell $(CXX) -dumpversion)

ifeq (3.,$(findstring 3.,$(CXX_VERSION)))
 use_strict_prototype=no
else
 use_strict_prototype=yes
endif

CFLAGS  += -w 
           -D_MIT_POSIX_THREADS -D_POSIX_THREADS -D_POSIX_THREAD_SAFE_FUNCTIONS

ifeq ($(use_strict_prototype),yes)
  CFLAGS  += -fno-strict-prototypes
endif

CCFLAGS += $(CFLAGS) -fno-implicit-templates
DCFLAGS += -g
CDLD    = $(CC)
DLD     = $(CXX)
LD      = $(CXX)
LIBS    += -lpthreads
OCFLAGS += -O2
PIC     = -fPIC
AR      = ar
ARFLAGS = rsuv
RANLIB  = @true
SOFLAGS = $(CPPFLAGS) -shared
SOBUILD = $(COMPILE.cc) $(PIC) -o $(VSHDIR)$*.so $<
PRELIB  = @true
