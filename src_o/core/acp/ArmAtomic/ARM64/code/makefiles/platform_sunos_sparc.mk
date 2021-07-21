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

LN_SF = ln -f -s

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
MCC32_FLAGS           = -g
MLD32_FLAGS           = -L/opt/SUNWspro/lib
MSO32_FLAGS           = -L/opt/SUNWspro/lib -G
M64_FLAGS             = -xarch=v9 -mt -xildoff
MCC64_FLAGS           = +w -g
MLD64_FLAGS            = -L/opt/SUNWspro/lib/v9 -L/usr/lib/sparcv9
MSO64_FLAGS            = -L/opt/SUNWspro/lib/v9 -L/usr/lib/sparcv9 -G

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
CC     = cc
CXX    = CC
LD_CC  = cc
LD_CXX = CC
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
#  DEP_FLAGS is -xM1, but not works properly.
#  The output format should be $(TARGET_DIR)/*.o : *.c
#  The automatic dependency generator should be written in the makefile
#  The command might be in the form of 
#   $ cc -xM1 acpInit.c -I ../include/ | gawk '{print "$(TARGET_DIR)" $1 " " $2 }'
#
COMP_FLAGS             = -c
PREP_FLAGS             = -E
DEP_FLAGS              = -xM1
DEP_CMD_CC             = $(CC) $(DEP_FLAGS) $(addprefix $(DEF_OPT),$(DEFINES)) $(INCLUDES) $< | gawk -F ":" '{fb="$(BUILD_DIR)/"substr($$1, 0, index($$1, ".o")-1); print fb"$(OBJ_SUF) " fb"$(SHOBJ_SUF) " fb".d : " $$2}' > $@
DEP_CMD_CXX            = $(CXX) $(DEP_FLAGS) $(addprefix $(DEF_OPT),$(DEFINES)) $(INCLUDES) $< | gawk -F ":" '{fb="$(BUILD_DIR)/"substr($$1, 0, index($$1, ".o")-1); print fb"$(OBJ_SUF) " fb"$(SHOBJ_SUF) " fb".d : " $$2}' > $@

DEFINES    = _POSIX_PTHREAD_SEMANTICS _POSIX_THREADS _POSIX_THREAD_SAFE_FUNCTIONS _REENTRANT _THREAD_SAFE

CC_FLAGS   =

CXX_FLAGS  = $(CC_FLAGS) -fno-rtti -fno-exceptions 
LD_FLAGS   =
LD_LIBS    =
AR_FLAGS   = -sruv
OUTFILE =
NM_FLAGS   = -t x
PIC_FLAGS  = -KPIC
SO_FLAGS   =
SO_LIBS    = -lm
COV_FLAGS  =

ifeq ($(SILENT_MODE),1)
OUTFILE = > /dev/null 2>&1
endif


#
# Platform dependent flags : Solaris
#
################ SOLARIS ####################
DEFINES_M32 += 
LD_LIBS     += -lpthread -ldl -lrt -lsocket -lnsl -lm -lgen -lposix4
SO_LIBS     += -lpthread -ldl -lrt -lsocket -lnsl -lm -lgen -lposix4

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

  CC_FLAGS += $(MCC$(compile_bit)_FLAGS)
  LD_FLAGS += $(MLD$(compile_bit)_FLAGS)
  SO_FLAGS += $(MSO$(compile_bit)_FLAGS)
else
  CC_FLAGS += $(M$(DEFAULT_COMPILE_BIT)_FLAGS)
  LD_FLAGS += $(M$(DEFAULT_COMPILE_BIT)_FLAGS)
  SO_FLAGS += $(M$(DEFAULT_COMPILE_BIT)_FLAGS)

  CC_FLAGS += $(MCC$(DEFAULT_COMPILE_BIT)_FLAGS)
  LD_FLAGS += $(MLD$(DEFAULT_COMPILE_BIT)_FLAGS)
  SO_FLAGS += $(MSO$(DEFAULT_COMPILE_BIT)_FLAGS)
endif

#
# readline library
#
READLINE_SUPPORT = yes
EXTLIBS += curses

#
# static analysis doing 
#

ifneq ($(static_anlz),)
  READLINE_SUPPORT = no
  DEFINES  += __STATIC_ANALYSIS_DOING__ ACP_CFG_MEMORY_CHECK
endif
