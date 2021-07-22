# Copyright 1999-2007, ALTIBASE Corporation or its subsidiaries.
# All rights reserved.

# $Id: Makefile 11299 2010-06-21 01:19:28Z djin $
#

include ../../../makefiles/root.mk
include $(CORE_DIR)/core.mk


OBJ_TARGETS      = aci_objs
SHOBJ_TARGETS    = aci_shobjs

aci_objs_SRCS    = $(ACI_SRCS)
aci_shobjs_SRCS  = $(ACI_SRCS)

INCLUDES        += $(INC_OPT).
DEFINES         += ACP_CFG_DL_EXPORT

# ACI does not follow ALINT coding-rule.
ALINT_IGNORES    = aciConv.c             	\
                   aciaciErrorMgr.c     	\
                   aciConvEuckr.c 			\
                   aciConvSjis.c 			\
                   aciMsgLog.c 				\
                   aciConv.c 				\
                   aciConvGb2312.c 			\
                   aciConvUhc1.c  			\
                   aciProperty.c			\
                   aciConvAscii.c			\
                   aciConvJisx0201.c  		\
                   aciConvUhc2.c  			\
                   aciVa.c					\
                   aciConvBig5.c   			\
                   aciConvJisx0208.c  		\
                   aciConvUtf8.c 			\
                   aciVarString.c			\
                   aciConvCp949.c 			\
                   aciConvJisx0212.c 		\
                   aciConvCp932.c     \
                   aciConvCp932ext.c  \
                   aciErrorMgr.c			\
                   aciVersion.c				\
                   aciConvEucjp.c			\
                   aciConvKsc5601.c 		\
                   aciConvCp936.c     \
                   aciConvCp936ext.c  \
                   aciConvGbk.c       \
                   aciConvGbkext1.c   \
                   aciConvGbkext2.c   \
                   aciConvGbkextinv.c \
                   aciHashUtil.c

include $(MAKEFILES_DIR)/rule.mk
