# Copyright 1999-2007, ALTIBASE Corporation or its subsidiaries.
# All rights reserved.

# $Id: platform_sunos.mk
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
AVAILABLE_COMPILER    = xlc_r
DEFAULT_COMPILER      = xlc_r

AVAILABLE_TOOL        = generic gcov insure purify quantify valgrind
DEFAULT_TOOL          = generic

AVAILABLE_BUILD_MODE  = debug release prerelease
DEFAULT_BUILD_MODE    = debug

PLATFORM_EXTERNAL_INCLUDES = $(CORE_INCLUDES)/external

AVAILABLE_COMPILE_BIT = 32 64
DEFAULT_COMPILE_BIT   = $(ALTI_CFG_BITTYPE)
MCC32_FLAGS           = -q32 -qtls=global-dynamic
MLD32_FLAGS           = -q32 -qtls=global-dynamic
MSO32_FLAGS           = -q32 -G -qtls=global-dynamic
M64_FLAGS             = 
MCC64_FLAGS           = -q64 -qtls=global-dynamic
MLD64_FLAGS           = -q64 -qtls=global-dynamic
MSO64_FLAGS           = -q64 -G -qtls=global-dynamic

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

#
# Compiler programs
#
CC     = $(DEFAULT_COMPILER)
CXX    = xlC_r
LD_CC  = xlc_r
LD_CXX = xlC_r
LD     = $(LD_CC)
AR     = ar
NM     = nm
COV    = gcov

#
# Compiler options
#
CC_OUT_OPT     = -o
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
#  DEP_FLAGS is -xM1, but not works properly.
#  The output format should be $(TARGET_DIR)/*.o : *.c
#  The automatic dependency generator should be written in the makefile
#  The command might be in the form of 
#   $ cc -xM1 acpInit.c -I ../include/ | gawk '{print "$(TARGET_DIR)" $1 " " $2 }'
#

COMP_FLAGS = -c
PREP_FLAGS = -E
DEP_FLAGS  = -qmakedep -qsyntaxonly
DEFINES    = _POSIX_PTHREAD_SEMANTICS _REENTRANT _THREAD_SAFE

CC_FLAGS   = -g

CXX_FLAGS  = $(CC_FLAGS) -fno-rtti -fno-exceptions
LD_FLAGS   = -brtl -bexpall
LD_LIBS    = -lm
AR_FLAGS   = -sruv
OUTFILE =
NM_FLAGS   = 
PIC_FLAGS  = -qmkshrobj
SO_FLAGS   = -brtl -bexpall
SO_LIBS    = 
COV_FLAGS  =

DEP_CMD_CC        = $(CC)  $(DEP_FLAGS) -MF $@ $< $(CC_FLAGS) $(addprefix $(DEF_OPT),$(DEFINES)) $(INCLUDES) -o $(addsuffix $(OBJ_SUF), $(basename $@))

ifeq ($(SILENT_MODE),1)
OUTFILE = > /dev/null 2>&1
endif


#
# Platform dependent flags : Solaris
#
################ AIX ####################
DEFINES_M32 += _LARGE_FILES
LD_LIBS     += 
SO_LIBS     += 

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
ifeq ($(compile_bit),)
  compile_bit=$(DEFAULT_COMPILE_BIT)
endif

ifeq ($(compile_bit),32)
  GAS       = gcc -x assembler -c
  GASSH     = $(GAS)
  AS        = as -a32
  ASSH      = as -a32
  AR_FLAGS += -X32
else
  GAS       = gcc -maix64 -x assembler -c
  GASSH     = $(GAS)
  AS        = as -a64
  ASSH      = as -a64
  AR_FLAGS += -X64
endif

CC_FLAGS += $(M$(compile_bit)_FLAGS)
LD_FLAGS += $(M$(compile_bit)_FLAGS)
SO_FLAGS += $(M$(compile_bit)_FLAGS)

CC_FLAGS += $(MCC$(compile_bit)_FLAGS)
LD_FLAGS += $(MLD$(compile_bit)_FLAGS)
SO_FLAGS += $(MSO$(compile_bit)_FLAGS)

#
# readline library
#
READLINE_SUPPORT = yes
EXTLIBS += curses m

#
# static analysis doing 
#

ifneq ($(static_anlz),)
  READLINE_SUPPORT = no
  DEFINES  += __STATIC_ANALYSIS_DOING__ ACP_CFG_MEMORY_CHECK
endif
