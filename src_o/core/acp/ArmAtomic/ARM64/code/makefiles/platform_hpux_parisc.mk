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
AVAILABLE_COMPILER    = cc
DEFAULT_COMPILER      = cc

AVAILABLE_TOOL        = generic gcov insure purify quantify valgrind
DEFAULT_TOOL          = generic

AVAILABLE_BUILD_MODE  = debug release prerelease
DEFAULT_BUILD_MODE    = debug

PLATFORM_EXTERNAL_INCLUDES = $(CORE_INCLUDES)/external

AVAILABLE_COMPILE_BIT = 32 64
DEFAULT_COMPILE_BIT   = $(ALTI_CFG_BITTYPE)

M32_FLAGS = +DD32 #+DA1.1 +DAportable # +DD32 sometimes generates PARISC2.0 format.
M64_FLAGS = +DD64


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
SHLIB_SUF = .sl

#
# Compiler programs
#
CC     = cc
CXX    = aCC
LD_CC  = cc
LD_CXX = aCC
LD     = $(LD_CC)
AR     = ar
NM     = nm
COV    = gcov

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
# Compiler flags
#
# to enable pthread (described in manpage for each platform)
#   HPUX    : -D_POSIX_C_SOURCE=199506L -lpthread
#
# -pthread flag is synonym for
#   HPUX    : -D_POSIX_C_SOURCE=199506L -D_REENTRANT -D_THREAD_SAFE -lpthread
#
COMP_FLAGS = -c
PREP_FLAGS = -E

#
# cc of HPUX-PARISC do not support generating dependency file.
#
DEP_FLAGS  =
DEP_CMD_CC		=
DEP_CMD_CXX		=

DEFINES    = _POSIX_PTHREAD_SEMANTICS _POSIX_THREADS _POSIX_THREAD_SAFE_FUNCTIONS _REENTRANT _THREAD_SAFE

CC_FLAGS   = -g
CXX_FLAGS  = $(CC_FLAGS) -fno-rtti -fno-exceptions 
LD_FLAGS   =
LD_LIBS    = -lm
AR_FLAGS   = -sruv
OUTFILE =
NM_FLAGS   = -t x
PIC_FLAGS  = +Z
SO_FLAGS   = -b -mt -n
SO_LIBS    = -lm
COV_FLAGS  =

ifeq ($(SILENT_MODE),1)
OUTFILE = > /dev/null 2>&1
endif


################ HPUX-PARISC ####################
DEFINES_M32 += _FILE_OFFSET_BITS=64

SO_FLAGS    += -Wl,+s # use the path list defined by the SHLIB_PATH
LD_FLAGS    += -Wl,+s # use the path list defined by the SHLIB_PATH
DEFINES     += _POSIX_C_SOURCE=199506L
# BUG-30841
# PARISC platform is using cl library to trace callstack.
# Becuase of the -lcl option, every program using Alticore should link cl library.
# Some programs that we do not build like JDBC does not link cl library,
# so that we cannot use JDBC in HP platform.
# Therefore we link the cl library statically.
#ifeq ($(AVAILABLE_COMPILE_BIT),32)

ifeq ($(compile_bit),32)
  LD_LIBS     += /usr/lib/libcl.a
else
  LD_LIBS     += /usr/lib/pa20_64/libcl.a
  SO_LIBS     += /usr/lib/pa20_64/libcl.a
endif

ifeq ($(ALTI_CFG_OS_MAJOR),11)
  ifeq ($(ALTI_CFG_OS_MINOR),11)
    DEFINES_M32 += _LARGEFILE_SOURCE
  endif
endif

LD_LIBS     += -lpthread -ldld -lrt
SO_LIBS     += -lpthread -ldld -lrt


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
  CC_FLAGS +=
endif

#
# Compile bit
#
ifneq ($(compile_bit),)
  CC_FLAGS += $(M$(compile_bit)_FLAGS)
  LD_FLAGS += $(M$(compile_bit)_FLAGS)
  SO_FLAGS += $(M$(compile_bit)_FLAGS)
else
  CC_FLAGS += $(M$(DEFAULT_COMPILE_BIT)_FLAGS)
  LD_FLAGS += $(M$(DEFAULT_COMPILE_BIT)_FLAGS)
  SO_FLAGS += $(M$(DEFAULT_COMPILE_BIT)_FLAGS)
endif

#
# readline library
#
READLINE_SUPPORT = yes
EXTLIBS += curses
DEFINES += _HPUX_SOURCE

#
# static analysis doing 
#
ifneq ($(static_anlz),)
  READLINE_SUPPORT = no
  DEFINES  += __STATIC_ANALYSIS_DOING__ ACP_CFG_MEMORY_CHECK
endif
