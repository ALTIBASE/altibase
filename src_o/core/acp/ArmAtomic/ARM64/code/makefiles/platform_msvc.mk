# Copyright 1999-2007, ALTIBASE Corporation or its subsidiaries.
# All rights reserved.

# $Id: platform_msvc.mk 11324 2010-06-23 06:19:48Z djin $
#

#
# Command for Windows platform
#

SHELL = cmd.exe

exec  = $(subst /,\,$(1))
CP    = $(call exec,$(UTILS_DIR)/windows/cp)
CP_R  = $(call exec,$(UTILS_DIR)/windows/cp) -R
MKDIR = $(call exec,$(UTILS_DIR)/windows/mkdir) -p
MV    = $(call exec,$(UTILS_DIR)/windows/mv) -f
RM    = $(call exec,$(UTILS_DIR)/windows/rm) -f
RMDIR = $(call exec,$(UTILS_DIR)/windows/rm) -rf
TOUCH = $(call exec,$(UTILS_DIR)/windows/touch)
CAT   = $(call exec,$(UTILS_DIR)/windows/cat)
DIFF  = $(call exec,$(UTILS_DIR)/windows/diff)
ECHO  = $(call exec,$(UTILS_DIR)/windows/echo)

PLATFORM_MK_INCLUDED = 1

#
# Variant selection
#
AVAILABLE_COMPILER    = msvc
DEFAULT_COMPILER      = msvc

AVAILABLE_TOOL        = generic
DEFAULT_TOOL          = generic

AVAILABLE_BUILD_MODE  = debug release prerelease
DEFAULT_BUILD_MODE    = debug

ifneq ($(findstring x64,$(shell cl 2>&1)),)
AVAILABLE_COMPILE_BIT = 64
DEFAULT_COMPILE_BIT   = $(ALTI_CFG_BITTYPE)
else ifneq ($(findstring 80x86,$(shell cl 2>&1)),)
AVAILABLE_COMPILE_BIT = 32
DEFAULT_COMPILE_BIT   = $(ALTI_CFG_BITTYPE)
else
$(error Unknown Microsoft Compiler)
endif

#
# File name generation
#
OBJ_SUF   = .obj
SHOBJ_SUF = _dl.obj
LIB_PRE   =
LIB_SUF   = _static.lib
SHLIB_PRE =
SHLIB_SUF = .dll
EXEC_SUF  = .exe
COV_SUF   =

#
# Compiler Programs
#
CC     = cl /nologo /TC
CXX    = cl /nologo /TP
LD_CC  = link /nologo
LD_CXX = link /nologo
LD     = $(LD_CC)
AR     = lib /nologo
NM     =
COV    =

#
# Compiler options
#
CC_OUT_OPT    = /Fo
LD_OUT_OPT    = /OUT:
AR_OUT_OPT    = /OUT:
INC_OPT       = /I
DEF_OPT       = /D
LIBDIR_OPT    = /LIBPATH:
LIBDEF_OPT    = /DEF:
LIB_OPT       =
LIB_AFT       = .lib
MAP_OPT       = /MAP:
COV_DIR_OPT   =

#
# Compiler flags
#
COMP_FLAGS = /c
PREP_FLAGS = /E
DEP_FLAGS  =

DEFINES    = _CRT_NONSTDC_NO_DEPRECATE _CRT_SECURE_NO_DEPRECATE
CC_FLAGS   = /W3 /EHsc
CXX_FLAGS  = $(CC_FLAGS)
LD_FLAGS   = /DEBUG /INCREMENTAL:NO /STACK:2097152 /OPT:REF /OPT:ICF
LD_LIBS    = netapi32.lib advapi32.lib ws2_32.lib iphlpapi.lib dbghelp.lib shell32.lib user32.lib
AR_FLAGS   =
NM_FLAGS   =
PIC_FLAGS  =
SO_FLAGS   = /DLL /DEBUG /INCREMENTAL:NO /OPT:REF /OPT:ICF
SO_LIBS    = netapi32.lib advapi32.lib ws2_32.lib iphlpapi.lib dbghelp.lib shell32.lib user32.lib
COV_FLAGS  =

ifneq ($(findstring /MANIFESTFILE:filename,$(shell link)),)
USE_MANIFEST  = 1
LD_FLAGS     += /MANIFEST /MANIFESTFILE:$$@.manifest /MAP:$$@.map /MAPINFO:EXPORTS
SO_FLAGS     += /MANIFEST /MANIFESTFILE:$$@.manifest /MAP:$$@.map /MAPINFO:EXPORTS
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
CC_FLAGS  += $(OCFLAGS)
else ifeq ($(BUILD_MODE),prerelease)
CC_FLAGS  += $(OCFLAGS) /Z7
else
CC_FLAGS  += /MTd /Od /Z7
endif

#
# Compile bit
#
ifeq ($(AVAILABLE_COMPILE_BIT),64)
LD_FLAGS += /MACHINE:X64
SO_FLAGS += /MACHINE:X64
else ifeq ($(AVAILABLE_COMPILE_BIT),32)
LD_FLAGS += /MACHINE:X86
SO_FLAGS += /MACHINE:X86
endif

#
# readline library
#

READLINE_SUPPORT = no

#
# static analysis doing 
#

ifneq ($(static_anlz),)
  DEFINES  += __STATIC_ANALYSIS_DOING__ ACP_CFG_MEMORY_CHECK
endif

