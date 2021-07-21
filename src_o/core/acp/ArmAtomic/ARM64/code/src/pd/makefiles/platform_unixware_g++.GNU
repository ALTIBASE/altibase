# platform_unixware_g++.GNU,v 4.18 1997/11/09 00:23:23 schmidt Exp

# UnixWare V2.2.1.2 with g++ version 2.7.2.2 *not* using Orbix

debug = 1

CC              = gcc
CXX             = g++
CFLAGS          += -D__STDC__=0
CCFLAGS         += $(CFLAGS) -fno-implicit-templates
DCFLAGS         += -g
CDLD            = $(CC)
DLD             = $(CXX)
LD              = $(CXX)
LIBS            += -lsocket -lnsl -lgen -lthread
PIC             = -fpic
AR              = ar
ARFLAGS         = ruv
RANLIB          = /bin/true
#SOFLAGS        = -assert pure-text
SOFLAGS         = -shared
# SOBUILD - compile into .so directly
SOBUILD         = $(COMPILE.cc) $(PIC) -o $(VSHDIR)$*.so $<
# SOLINK        = $(SOLINK.cc) -o $@ $(LDFLAGS) $<
PRELIB          = @true
