# platform_aix_g++.GNU,v 4.1 1998/10/30 01:04:50 shuston Exp
#
# Platform macros for building on AIX with g++.  This has been tested on
# AIX 4.1.4 and gcc 2.7.2.3.
#
# A note - later versions of gcc(>2.7.2.3, I don't know which) have a -mthreads
# flag which does the same job more elegantly.
# I haven't managed to get the shared library to work.
# I know having tli_r and tli is a bad idea, but it won't link otherwise.
#
# Contributed by Ian Wright <I.Wright@elsevier.co.uk>

# controlled by env.mk - by jdlee
#debug = 1
#static_libs_only = 1

# added by jdlee
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

CC              = gcc
CXX             = gcc
CFLAGS          += -W -Wall -mcpu=common -D_POSIX_THREADS -D_POSIX_THREAD_SAFE_FUNCTIONS -D_POSIX_PTHREAD_SEMANTICS -DPDL_LACKS_RTTI -D_REENTRANT -fno-rtti
CCFLAGS         += $(CFLAGS) -fno-implicit-templates
DCFLAGS         += -g
CDLD            = $(CC)
DLD             = $(CXX)
LD              = $(CXX)
OCFLAGS         = -O3
OCCFLAGS        = $(OCFLAGS) $(added_optimizer_flag)
LIBS            += -lpthreads
PIC             = -fPIC
ARFLAGS         = cruv
RANLIB          = ranlib
