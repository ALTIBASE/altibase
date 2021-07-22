# Copyright 1999-2007, ALTIBASE Corporation or its subsidiaries.
# All rights reserved.

# $Id: platform_gcc.mk 11324 2010-06-23 06:19:48Z djin $
#

#
# Command for Unix platforms
#
exec  = $(1)
CP    = cp
CP_R  = cp -R
MKDIR = mkdir -p
MV    = mv
RM    = rm -f
RMDIR = rm -rf
TOUCH = touch
CAT   = cat
DIFF  = diff
ECHO  = echo

LN_SF = ln -s -f

PLATFORM_MK_INCLUDED = 1

#
# Variant selection
#
AVAILABLE_COMPILER    = gcc
DEFAULT_COMPILER      = gcc

AVAILABLE_TOOL        = generic gcov insure purify quantify valgrind
DEFAULT_TOOL          = generic

AVAILABLE_BUILD_MODE  = debug release prerelease
DEFAULT_BUILD_MODE    = debug

PLATFORM_EXTERNAL_INCLUDES = $(CORE_INCLUDES)/external

ifeq ($(ALTI_CFG_CPU),ALPHA)
  AVAILABLE_COMPILE_BIT = 64
  DEFAULT_COMPILE_BIT   = $(ALTI_CFG_BITTYPE)
else ifeq ($(ALTI_CFG_CPU),IA64)
  ifeq ($(ALTI_CFG_OS),HPUX)
    AVAILABLE_COMPILE_BIT = 32 64
    DEFAULT_COMPILE_BIT   = $(ALTI_CFG_BITTYPE)
    M32_FLAGS             = -milp32
    M64_FLAGS             = -mlp64
  else
    AVAILABLE_COMPILE_BIT = 64
    DEFAULT_COMPILE_BIT   = $(ALTI_CFG_BITTYPE)
  endif
else ifeq ($(ALTI_CFG_CPU),PARISC)
  ifeq ($(filter hppa64-%,$(shell gcc -dumpmachine)),)
    AVAILABLE_COMPILE_BIT = 32
    DEFAULT_COMPILE_BIT   = $(ALTI_CFG_BITTYPE)
  else
    AVAILABLE_COMPILE_BIT = 64
    DEFAULT_COMPILE_BIT   = $(ALTI_CFG_BITTYPE)
  endif
else ifeq ($(ALTI_CFG_CPU),POWERPC)
  ifeq ($(ALTI_CFG_OS),AIX)
    AVAILABLE_COMPILE_BIT = 32 64
    DEFAULT_COMPILE_BIT   = $(ALTI_CFG_BITTYPE)
    M32_FLAGS             = -Wa,-many -maix32
    M64_FLAGS             = -maix64
  else ifeq ($(ALTI_CFG_OS),LINUX)
    AVAILABLE_COMPILE_BIT = 64
    DEFAULT_COMPILE_BIT   = $(ALTI_CFG_BITTYPE)
    M32_FLAGS             = -Wa,-many -mpowerpc
    M64_FLAGS             = -mpowerpc64
    #M32_FLAGS             = -Wa,-many -mtune=power7
    #M64_FLAGS             = -mtune=power7
  endif
else ifeq ($(ALTI_CFG_CPU),SPARC)
  AVAILABLE_COMPILE_BIT = 32 64
  DEFAULT_COMPILE_BIT   = $(ALTI_CFG_BITTYPE)
  M32_FLAGS             = -m32 -mcpu=v9
  M64_FLAGS             = -m64
else ifeq ($(ALTI_CFG_CPU),X86)
  ifeq ($(ALTI_CFG_BITTYPE),64)
    AVAILABLE_COMPILE_BIT = 32 64
    DEFAULT_COMPILE_BIT   = $(ALTI_CFG_BITTYPE)
    M32_FLAGS             = -m32 -mtune=i686
    M64_FLAGS             = -m64 -mtune=k8
  else
    AVAILABLE_COMPILE_BIT = 32
    DEFAULT_COMPILE_BIT   = $(ALTI_CFG_BITTYPE)
    M32_FLAGS             = -mtune=i686
  endif
else
  $(error Unknown CPU)
endif

#
# File name generation
#
OBJ_SUF   = .o
SHOBJ_SUF = _pic.o
LIB_PRE   = lib
LIB_SUF   = .a
SHLIB_PRE = lib
SHLIB_SUF = .so
EXEC_SUF  =
COV_SUF   = .gcov

ifeq ($(ALTI_CFG_OS),AIX)
  SHOBJ_SUF = .o
else ifeq ($(ALTI_CFG_OS),HPUX)
  SHLIB_SUF = .sl
else ifeq ($(ALTI_CFG_OS),DARWIN)
  SHLIB_SUF = .dylib
endif

#
# Compiler programs
#
CC     = gcc
CXX    = g++
LD_CC  = gcc
LD_CXX = g++
LD     = $(LD_CC)
AR     = ar
NM     = nm
COV    = gcov
GCC_VERSION := $(shell $(CC) -dumpversion)

#
# Compiler options
#
CC_OUT_OPT     = -o
CC_OUT_OPT    +=
LD_OUT_OPT     = $(CC_OUT_OPT)
AR_OUT_OPT     =
INC_OPT        = -I
DEF_OPT        = -D
LIBDIR_OPT     = -L
LIBDEF_OPT     =
LIB_OPT        = -l
LIB_AFT        =
MAP_OPT        =
COV_DIR_OPT    = -o
COV_DIR_OPT   +=

#
# Compile bit setting
#

ifeq ($(compile_bit),)
	compile_bit = $(DEFAULT_COMPILE_BIT)
endif

#
# Compiler flags
#
# to enable pthread (described in manpage for each platform)
#   AIX     : -D_THREAD_SAFE -lpthreads
#   HPUX    : -D_POSIX_C_SOURCE=199506L -lpthread
#   LINUX   : -lpthread
#   SOLARIS : -D_REENTRANT -lpthread
#   TRU64   : -D_REENTRANT -lpthread
#
# -pthread flag is synonym for
#   AIX     : -D_THREAD_SAFE -lpthreads
#   HPUX    : -D_POSIX_C_SOURCE=199506L -D_REENTRANT -D_THREAD_SAFE -lpthread
#   LINUX   : -D_REENTRANT -lpthread
#   SOLARIS : -D_REENTRANT -lpthread
#   TRU64   : -D_REENTRANT -lpthread
#
COMP_FLAGS = -c
PREP_FLAGS = -E
DEP_FLAGS  = -MM -MT '$(sort $(addsuffix $(OBJ_SUF),$(basename $@)) $(addsuffix $(SHOBJ_SUF), $(basename $@))) $@'
DEP_CMD_CC		= $(CC) $(DEP_FLAGS) $(CC_FLAGS) $(addprefix $(DEF_OPT),$(DEFINES)) $(INCLUDES) $< > $@
DEP_CMD_CXX		= $(CXX) $(DEP_FLAGS) $(CC_FLAGS) $(addprefix $(DEF_OPT),$(DEFINES)) $(INCLUDES) $< > $@

#DEFINES    =  _POSIX_THREAD_SAFE_FUNCTIONS _REENTRANT _THREAD_SAFE
DEFINES    =  

CC_FLAGS   = -g -W -Wall

ifeq ($(ALTI_CFG_OS),AIX)
 #if it is a nil, DEFAULT_COMPILE_BIT is 64
 ifeq ($(compile_bit),)
  CC_FLAGS = -gxcoff -W -Wall
 endif
 ifeq ($(compile_bit),64)
  CC_FLAGS = -gxcoff -W -Wall
 endif
endif

CXX_FLAGS  = $(CC_FLAGS) -fno-rtti -fno-exceptions 
LD_FLAGS   =
LD_LIBS    = -lm
AR_FLAGS   = -sruv
OUTFILE    =
NM_FLAGS   = -t x
PIC_FLAGS  =
SO_FLAGS   = -shared -static-libgcc
SO_LIBS    = -lm
COV_FLAGS  =

ifeq ($(SILENT_MODE),1)
OUTFILE = > /dev/null 2>&1
endif


# BUG-21826
# On IA64 HPUX, gcc has a huge optimization bugs in memcpy(). we need to escape from this.
HPUX_MEMCPY_BUG_GCC_VERSION= $(or $(findstring 4.3.1,$(GCC_VERSION)) $(findstring 4.1.2,$(GCC_VERSION)) )

#
# Platform dependent flags
#
################ AIX ####################
ifeq ($(ALTI_CFG_OS),AIX)
  LD_OUT_OPT  :=   -Xlinker -brtl $(LD_OUT_OPT) # link shared library prito to static one
  SO_FLAGS    +=   -Xlinker -brtl  # link shared library prito to static one
  LD_FLAGS    +=   -Xlinker -bexpall 
  DEFINES_M32 += _LARGE_FILES
  AR_FLAGS    = -X32_64 -sruv
  NM_FLAGS    = -X32_64 -t x
  LD_LIBS     += -lpthreads
  SO_LIBS     += -lpthreads

  ifeq ($(compile_bit),32)
    GAS                   = gcc -x assembler -c
    GASSH                 = $(GAS)
  else
    GAS                   = gcc -maix64 -x assembler -c
    GASSH                 = $(GAS)
  endif


################ LINUX ####################
else ifeq ($(ALTI_CFG_OS),LINUX)
  DEFINES_M32 += _FILE_OFFSET_BITS=64
  DEFINES     += _GNU_SOURCE
  CC_FLAGS	  += -rdynamic
  SO_FLAGS    += -rdynamic
  LD_LIBS     += -lpthread -ldl -rdynamic
  SO_LIBS     += -lpthread -ldl
  ifeq ($(compile_bit),32)
    GAS                   = gcc -mpowerpc  -x assembler -c
    GASSH                 = $(GAS)
  else
    GAS                   = gcc -mpowerpc64  -x assembler -c
    GASSH                 = $(GAS)
  endif

endif

#
# Tool
#
ifeq ($(tool),gcov)
  CC_FLAGS += -fprofile-arcs -ftest-coverage
  LD_LIBS  += -lgcov
else ifeq ($(tool),insure)
  CC_TOOL   = insure
  CXX_TOOL  = insure
  LD_TOOL   = insure
  DEFINES  += ACP_CFG_MEMORY_CHECK
else ifeq ($(tool),purify)
  LD_TOOL   = purify
  DEFINES  += ACP_CFG_MEMORY_CHECK
else ifeq ($(tool),quantify)
  LD_TOOL   = quantify
else ifeq ($(tool),valgrind)
  TEST_TOOL = valgrind --tool=memcheck --trace-children=yes --track-fds=yes --leak-check=full --leak-resolution=high --show-reachable=yes
  DEFINES  += ACP_CFG_MEMORY_CHECK
endif

#
# Trace mode
#
ifeq ($(TRACE_ON), 1)
  CC_FLAGS += -DACP_CFG_TRACE
endif

#
# Build mode
#
ifeq ($(BUILD_MODE),release)
  CC_FLAGS += $(OCFLAGS)
else ifeq ($(BUILD_MODE),prerelease)
  CC_FLAGS += $(OCFLAGS)
else
  CC_FLAGS += -fno-inline
endif

#
# Compile bit
#
CC_FLAGS += $(M$(compile_bit)_FLAGS)
LD_FLAGS += $(M$(compile_bit)_FLAGS)
SO_FLAGS += $(M$(compile_bit)_FLAGS)

#
# readline library
#

ifeq ($(ALTI_CFG_OS),LINUX)
  READLINE_SUPPORT = yes
  EXTLIBS += ncurses
else ifeq ($(ALTI_CFG_OS),SOLARIS)
  READLINE_SUPPORT = yes
  EXTLIBS += curses
else ifeq ($(ALTI_CFG_OS),HPUX)
  READLINE_SUPPORT = yes
  EXTLIBS += curses
else ifeq ($(ALTI_CFG_OS),TRU64)
  READLINE_SUPPORT = yes
  EXTLIBS += curses
else ifeq ($(ALTI_CFG_OS),AIX)
  READLINE_SUPPORT = yes
  EXTLIBS += curses
else
  READLINE_SUPPORT = no
endif

#
# static analysis doing 
#

ifneq ($(static_anlz),)
  READLINE_SUPPORT = no
  DEFINES  += __STATIC_ANALYSIS_DOING__ ACP_CFG_MEMORY_CHECK
endif
