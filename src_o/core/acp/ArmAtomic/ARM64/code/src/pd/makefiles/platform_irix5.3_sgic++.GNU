# platform_irix5.3_sgic++.GNU,v 4.5 1997/09/10 16:47:18 levine Exp

# Irix 5.3 with SGI C++

# This suppresses common compiler warnings which appear in the 
# PDL code but should not matter.  The warnings can be turned on
# again by removing the -woff clause in the CPPFLAGS definition.

debug = 1

CC		= cc
CXX		= CC
CCFLAGS		+= $(CFLAGS)
DCFLAGS		+= -g
CDLD		= $(CC)
DLD		= $(CXX)
LD		= $(CXX) 
CPPFLAGS        += -ptused -prelink +pp -woff 3203,3209,3161,3262,3665
LDFLAGS		+= -rpath "$(PD_DIR)/pdl"
LIBS		= 
PIC		= -KPIC
AR		= ar
ARFLAGS		= r
RANLIB		= echo
SOFLAGS		= -shared $(CPPFLAGS) -all
SOBUILD		= $(COMPILE.cc) $(PIC) -o $(VSHDIR)$*.o $<; \
                  $(SOLINK.cc) -o $@ $(LDFLAGS) $(VSHDIR)$*.o
