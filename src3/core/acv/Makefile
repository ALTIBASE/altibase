# Copyright 1999-2007, ALTIBASE Corporation or its subsidiaries.
# All rights reserved.

# $Id: Makefile 3531 2008-11-07 03:37:07Z sjkim $
#

include ../../../makefiles/root.mk
include $(CORE_DIR)/core.mk

EXEC_TARGETS	   = acvLicenseMaker \
					 acvInstall

acvLicenseMaker_NAME	= acvLicenseMaker
acvLicenseMaker_SRCS	= acvLicenseMaker.c
acvLicenseMaker_LIBS	= $(CORE_LIBS)

acvInstall_NAME			= acvInstall
acvInstall_SRCS			= acvInstall.c
acvInstall_LIBS			= $(CORE_LIBS)

DEFINES           += ACP_CFG_DL_STATIC

include $(MAKEFILES_DIR)/rule.mk
