# platform_mvs.GNU,v 4.10 1999/06/02 22:52:25 coryan Exp

# For MVS OpenEdition platform

debug = 0
 
CXX           = cxx
DCFLAGS	      += -g
DCCFLAGS      += -g
CPPFLAGS      += -+
DEFFLAGS      = -D_ALL_SOURCE -DNDEBUG
CC            = $(CXX)
LD            = $(CXX)
DLD           = $(CXX)
AR            = ar
ARFLAGS       = ruv
RANLIB        = echo
#LDFLAGS      = -W l,p,map
 
# MVCMD needed because cxx does not use the -o option to plpdl objects
MVCMD         = @test ! -s $(@F) || mv $(@F) $(@D)/$(@F)
 
SOFLAGS       = -W l,dll
PIC           = -W c,exportall
static_libs   = 1
