# platform_m88k.GNU,v 4.15 1997/09/22 00:37:57 levine Exp

# SunOS 5.x (Solaris 2.x) with SunC++ 4.x
# *not* using Orbix

debug = 1
optimize = 1

CC		= gcc
CXX             = g++

CXX_VERSION := $(shell $(CXX) -dumpversion)

ifeq (3.,$(findstring 3.,$(CXX_VERSION)))
 use_strict_prototype=no
else
 use_strict_prototype=yes
endif

CFLAGS		+= -w
CCFLAGS		+= $(CFLAGS) -D__m88k__ -fno-implicit-templates

ifeq ($(use_strict_prototype),yes)
  CCFLAGS  += -fno-strict-prototypes
endif

DCFLAGS		+= -g
CDLD		= $(CC) 
DLD		= $(CXX) 
LD		= $(CXX) 
INCLDIRS	+= -I$(PD_DIR) -I.
INCLDIRS	+= -I/devel/rmm/v1.0/local/src/threads/include
LDFLAGS		+= -L $(PD_DIR)/pdl -L ./
LDFLAGS		+= -L/devel/rmm/v1.0/local/src/threads/lib
LIBS		+= -lsocket -ldl -lnsl -lgen -lstdc++ -lg++
LIBS		+= -lgthreads -lgmalloc
OCFLAGS		+= -O2
PIC		= -fpic
AR		= ar 
ARFLAGS		= ruv 
RANLIB		= /bin/true 
SOFLAGS		= -G $(CPPFLAGS)
SOBUILD		= $(COMPILE.cc) $(PIC) -o $(VSHDIR)$*.o $<; \
		  $(SOLINK.cc) -o $@ -h $@ $(LDFLAGS) $(VSHDIR)$*.o
PRELIB	= @true
