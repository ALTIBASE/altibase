# platform_aix4_cset++.GNU,v 1.6 1999/07/21 16:35:59 shuston Exp

# AIX 4.x using the C Set++ compiler.
# *not* using Orbix
#
# On 4.1, this file assumes that the user has installed the AIX patch
# containing the dl*() APIs.  To use these APIs, IBM has created a
# separate product (free to AIX licensees) called shared library
# hookable symbols (or slhs/6000).  If they don't have this patch, the
# sv* commands for compiling and linking will not be present on the
# system.
#
# On 4.2, the AIX fileset bos.rte.bind_cmds must be at version 4.2.0.2 or
# higher in order to build libPDLshr.a, and at 4.2.0.4 or higher for dl*()
# routines to work properly. Best to apply the update available at:
# ftp://service.boulder.ibm.com/aix/fixes/v4/os/bos.rte.bind_cmds.4.2.0.4.bff
# or a successor.

ifeq (,$(debug))
  debug = 1
endif
ifeq (,$(distrib))
  distrib = 0
endif
ifeq (,$(optimize))
  optimize = 0
endif
ifeq (,$(threads))
  threads = 1
endif

# modified by jdlee
buildbits = 32
ifeq ($(compile64),1)
  buildbits = 64
else
  buildbits = 32
endif

# In case anything here or in the config depends on OS version number,
# grab it here and pass it all to the compiler as well.
AIX_MAJOR_VERS := $(shell uname -v)
AIX_MINOR_VERS := $(shell uname -r)

CC	    = xlc_r
CXX	    = xlC_r

# -qextchk is handy, but produces false type miidatches when linking
# netsvcs with 3.1.4, so it's disabled.  IBM reports this fixed in 3.6.4.
CFLAGS      += -qtls=global-dynamic
CCFLAGS     += $(CFLAGS) -qcheck=nobounds:divzero:nullptr -qtempinc -qinfo -D_POSIX_PTHREAD_SEMANTICS -D_REENTRANT -qfloat=nomaf

# IBM C/C++ compiler 3.6.x produces a bazillion warnings about 0-valued
# preprocessor defs. Since both 3.1 and 3.4 could be installed, don't ask
# lslpp for one or the other. Instead, compile a file and see which compiler
# the user has set up for use. This trick was submitted by Craig Rodrigues
# <rodrigc@mediaone.net>, originally from the vacpp compiler newsgroup.
# It relies on the preprocessor defining __xlC__ to the proper version
# number of the compiler.
XLCVERSION := $(shell echo "__xlC__" > ./testAIXCompilerVersion.cpp)
XLCVERSION := $(shell $(CXX) -E ./testAIXCompilerVersion.cpp | tail -1)
PDL_AIX_CLEAN := $(shell $(RM) ./testAIXCompilerVersion.cpp)
ifeq ($(XLCVERSION),0x0306)
 CCFLAGS    += -qflag=e:e
else
 CCFLAGS    += -qflag=w:w
endif
#CPPFLAGS    += -qlanglvl=ansi -DPDL_AIX_MAJOR_VERS=$(AIX_MAJOR_VERS) -DPDL_AIX_MINOR_VERS=$(AIX_MINOR_VERS)
CPPFLAGS    += -DPDL_AIX_MAJOR_VERS=$(AIX_MAJOR_VERS) -DPDL_AIX_MINOR_VERS=$(AIX_MINOR_VERS)

# -qinitauto seems useful, but when built with -qinitauto=5F, the
# Process_Strategy_Test went CPU bound in ostream, so I removed it (S. Huston)
DCFLAGS     += -g -DDEBUG

DLD	    = makeC++SharedLib_r
LD          = $(CXX)

ifeq ($(shared_libs),1)
SHLIBA      = $(SHLIB:lib%.so=lib%shr.a)
endif

ifeq ($(AIX_MINOR_VERS),1)
LLIBS	    = -lsvld -ltli_r
LIBS	    += $(filter-out $(SHLIBA:lib%.a=-l%), $(LLIBS))
else
LIBS	    +=
endif

AR	    = ar
LDFLAGS	    += -bI:/lib/pse.exp 
# Default OCCFLAGS builds common code for all RS/6000 architectures but
# this can be set to optimize for your particular computer. The commented
# out example optimizes for RS/6000 43P.
# OCCFLAGS is not used by default.  To used it, set optimize=1
# either in this file or on the command line.
#OCCFLAGS    += -qarch=ppc -qtune=604
#OCCFLAGS    += -qarch=com 
OCFLAGS     += -O3 -qstrict -qinline -qalias=noansi $(added_optimizer_flag)
OCCFLAGS    += -O3 -qstrict  -qinline -qalias=noansi $(added_optimizer_flag)
RANLIB	    = ranlib
SOFLAGS	    = -p 0
PIC         = -qmkshrobj
TEMPINCDIR  = tempinc
SOBUILD = 

XCC = -qcpluscmt

## added by jdlee
ifeq ($(buildbits),32)
CFLAGS     += -q32
ARFLAGS	    = -X32 ruv
LDFLAGS	    += -q32
else
  ifeq ($(buildbits),64)
    CFLAGS     += -q64
    ARFLAGS	    = -X64 ruv
    LDFLAGS	    += -q64
  endif
endif

