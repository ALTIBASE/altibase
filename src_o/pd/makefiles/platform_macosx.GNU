# platform_macosx.GNU,v 4.6 2001/08/14 18:57:28 bala Exp
# platform_macosx.GNU
# support for Mac OS X 10.1

CC              = cc
CXX             = c++
CFLAGS          += -Wall -Wpointer-arith -pipe \
           -D_POSIX_PTHREAD_SEMANTICS -D_POSIX_THREADS -D_POSIX_THREAD_SAFE_FUNCTIONS -D_REENTRANT

# Test for template instantiation.
#
include $(PD_DIR)/makefiles/platform_g++_common.GNU

CFLAGS  += -m64 -mtune=k8

CCFLAGS         += $(CFLAGS) $(TEMPLATES_FLAG)
DCFLAGS         += -g -DDEBUG
#DLD             = libtool
DLD     = $(CXX)

LD              = $(CXX)
LDFLAGS		+= -flat_namespace
LIBS            += -lcc_dynamic -lstdc++ -lSystem $(PDLLIB)
OCFLAGS         += -O2
RANLIB          = ranlib
SOEXT		= dylib
SOFLAGS = -dynamic -shared
SOBUILD = -o $(VSHDIR)$*.dylib $<

#by gurugio
CDLD            = $(CC)
PIC     = -fPIC
SOFLAGS = $(CPPFLAGS) -shared
SOBUILD = $(COMPILE.cc) $(PIC) -o $(VSHDIR)$*.o $<; \
          $(SOLINK.cc) -o $@ $(LDFLAGS) $(VSHDIR)$*.o

