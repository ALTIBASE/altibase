# Copyright 1999-2007, ALTIBASE Corporation or its subsidiaries.
# All rights reserved.

# $Id: Makefile 11331 2010-06-24 02:00:29Z vsebelev $
#

include $(ALTIDEV_HOME)/makefiles/root.mk
include $(CORE_DIR)/core.mk


EXEC_TARGETS       = sampleAcpOptGet         \
                     sampleActGenerateCore   \
                     sampleAcpFile           \
                     sampleAcpFileList       \
                     sampleAcpFileInfo		\
                     sampleAcpInet           \
					 sampleAcpProcDaemonize \
					 sampleAclReadLine	\
                     sampleAcpVersion \
                     sampleAclConf


sampleAcpOptGet_NAME          = sampleAcpOptGet
sampleAcpOptGet_SRCS          = sampleAcpOptGet.c
sampleAcpOptGet_LIBS          = $(CTEST_LIBS) $(CORE_LIBS)

sampleActGenerateCore_NAME    = sampleActGenerateCore
sampleActGenerateCore_SRCS    = sampleActGenerateCore.c
sampleActGenerateCore_LIBS    = $(CTEST_LIBS) $(CORE_LIBS)

sampleAcpFile_NAME    = sampleAcpFile
sampleAcpFile_SRCS    = sampleAcpFile.c
sampleAcpFile_LIBS    = $(CTEST_LIBS) $(CORE_LIBS)

sampleAcpFileList_NAME    = sampleAcpFileList
sampleAcpFileList_SRCS    = sampleAcpFileList.c
sampleAcpFileList_LIBS    = $(CTEST_LIBS) $(CORE_LIBS)

sampleAcpFileInfo_NAME    = sampleAcpFileInfo
sampleAcpFileInfo_SRCS    = sampleAcpFileInfo.c
sampleAcpFileInfo_LIBS    = $(CTEST_LIBS) $(CORE_LIBS)

sampleAcpInet_NAME    = sampleAcpInet
sampleAcpInet_SRCS    = sampleAcpInet
sampleAcpInet_LIBS    = $(CTEST_LIBS) $(CORE_LIBS)

sampleAcpProcDaemonize_NAME          = sampleAcpProcDaemonize
sampleAcpProcDaemonize_SRCS          = sampleAcpProcDaemonize.c
sampleAcpProcDaemonize_LIBS          = $(CTEST_LIBS) $(CORE_LIBS)

sampleAclReadLine_NAME          = sampleAclReadLine
sampleAclReadLine_SRCS          = sampleAclReadLine.c
sampleAclReadLine_LIBS          = $(CTEST_LIBS) $(CORE_LIBS)

sampleAcpVersion_NAME           = sampleAcpVersion
sampleAcpVersion_SRCS           = sampleAcpVersion.c
sampleAcpVersion_LIBS           = $(CTEST_LIBS) $(CORE_LIBS)

sampleAclConf_NAME           = sampleAclConf
sampleAclConf_SRCS           = sampleAclConf.c
sampleAclConf_LIBS           = $(CTEST_LIBS) $(CORE_LIBS)


DEFINES               += ACP_CFG_DL_STATIC

include $(MAKEFILES_DIR)/rule.mk

