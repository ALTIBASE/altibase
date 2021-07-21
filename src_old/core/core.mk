# Copyright 1999-2007, ALTIBASE Corporation or its subsidiaries.
# All rights reserved.

# $Id: core.mk 11275 2010-06-15 06:24:57Z djin $
#

ACP_SRCS       = \
                 acpCallstack.c        \
                 acpCallstackPrivate.c \
                 acpChar.c             \
                 acpCStr.c             \
                 acpCStrDouble.c       \
                 acpDir.c              \
                 acpDl.c               \
                 acpEnv.c              \
                 acpError.c            \
                 acpFeature.c          \
                 acpFile.c             \
                 acpInet.c             \
                 acpInet_IPv6Emul.c    \
                 acpInit.c             \
                 acpMath.c             \
                 acpMmap.c             \
                 acpMem.c              \
                 acpOS.c               \
                 acpOpt.c              \
                 acpPoll.c             \
                 acpPrintf.c           \
                 acpPrintfCore.c       \
                 acpPrintfRender.c     \
                 acpProc.c             \
                 acpProcMutex.c        \
                 acpSem.c              \
                 acpPset.c             \
                 acpSearch.c           \
                 acpShm.c              \
                 acpSignal.c           \
                 acpSock.c             \
                 acpSockConnect.c      \
                 acpSockPoll.c         \
                 acpSockRecv.c         \
                 acpSockSend.c         \
                 acpSpinLock.c         \
                 acpSpinWait.c         \
                 acpStd.c              \
                 acpStr.c              \
                 acpStrCmp.c           \
                 acpStrFind.c          \
                 acpSym.c              \
                 acpSys.c              \
                 acpSysMacAddr-UNIX.c  \
                 acpSysIPAddr-UNIX.c   \
                 acpSysHardware.c      \
                 acpTest.c             \
                 acpThr.c              \
                 acpThrBarrier.c       \
                 acpThrCond.c          \
                 acpThrMutex.c         \
                 acpThrRwlock.c        \
                 acpTime.c             \
                 acpVersion.c

ifeq ($(COMPILER),msvc)
# atomic for WINDOWS is in acpAtomic.h
  ACP_SRCS += acpMemBarrier.c
else
  ifeq ($(COMPILER),gcc)
    ACP_SRCS += acpAtomic.c acpMemBarrier.c
    ifeq ($(ALTI_CFG_CPU),POWERPC)
      ifeq ($(ALTI_CFG_OS),LINUX)
        # ACP_GASM_SRCS = acpAtomic-PPC16_64MODE_LINUX.gs
      else
        ifeq ($(compile_bit),32)
          ACP_GASM_SRCS = acpAtomic-PPC16_32MODE.gs
        else
          ACP_GASM_SRCS = acpAtomic-PPC16_64MODE.gs
        endif
      endif
    else ifeq ($(ALTI_CFG_CPU),IA64)
      ifeq ($(ALTI_CFG_OS),LINUX)
      else
        ifeq ($(compile_bit),32)
          ACP_GASM_SRCS = acpAtomic-IA64_CAS16_32MODE.gs
        else
          ACP_GASM_SRCS = acpAtomic-IA64_CAS16_64MODE.gs
        endif
      endif
    else
      ACP_GASM_SRCS =
    endif
  else
# Native C compilers
    ifeq ($(ALTI_CFG_CPU),PARISC)
      ACP_SRCS += acpAtomic-PARISC_USE_LOCKS.c
      ACP_ASM_SRCS =
    else ifeq ($(ALTI_CFG_CPU),POWERPC)
      ACP_SRCS += acpMemBarrier.c

      ifeq ($(ALTI_CFG_OS),LINUX)
        ifeq ($(compile_bit),32)
          ACP_GASM_SRCS = acpAtomic-PPC16_32MODE_LINUX.gs
          ACP_ASM_SRCS  = acpAtomic-PPC_32MODE_LINUX.s acpMemBarrier-PPC_32MODE_LINUX.s
        else
          ACP_GASM_SRCS = acpAtomic-PPC16_64MODE_LINUX.gs
          ACP_ASM_SRCS  = acpAtomic-PPC_64MODE_LINUX.s acpMemBarrier-PPC_64MODE_LINUX.s
        endif
      else
        ifeq ($(compile_bit),32)
          ACP_GASM_SRCS = acpAtomic-PPC16_32MODE.gs
          ACP_ASM_SRCS  = acpAtomic-PPC_32MODE.s acpMemBarrier-PPC_32MODE.s
        else
          ACP_GASM_SRCS = acpAtomic-PPC16_64MODE.gs
          ACP_ASM_SRCS  = acpAtomic-PPC_64MODE.s acpMemBarrier-PPC_64MODE.s
        endif
      endif
    endif
  endif
endif


ACE_SRCS       = aceAssert.c           \
                 aceException.c        \
                 aceMsgTable.c

ACL_GSRCS      = aclConfParse.c        \
                 aclConfLex.c

ACL_SRCS       = $(ACL_GSRCS)          \
                 aclConf.c             \
                 aclConfPrivate.c      \
                 aclHash.c             \
                 aclHashFunc.c         \
                 aclLog.c              \
                 aclMem.c              \
                 aclMemArea.c          \
                 aclMemPool.c          \
                 aclMemTlsf.c          \
                 aclMemTlsfImp.c       \
                 aclQueue.c            \
                 aclCompression.c      \
                 aclLZ4.c              \
                 aclReadLine.c         \
                 aclCodeUTF8.c         \
                 aclCryptTEA.c         \
                 aclLicFile.c          \
                 aclLicLicense.c       \
                 aclLFMemPool.c        \
                 aclSafeList.c         \
                 aclStack.c 

ifeq ($(ALTI_CFG_OS),LINUX)
ACL_SRCS      += aclZookeeper.c
endif

ACT_SRCS       = actDump.c             \
                 actTest.c             \
                 actPerf.c

ACI_SRCS       = aciVa.c               \
                 aciErrorMgr.c         \
                 aciProperty.c         \
                 aciVersion.c          \
                 aciHashUtil.c         \
                 aciConv.c             \
                 aciConvAscii.c        \
                 aciConvCp949.c        \
                 aciConvEucjp.c        \
                 aciConvEuckr.c        \
                 aciConvGb2312.c       \
                 aciConvJisx0201.c     \
                 aciConvJisx0208.c     \
                 aciConvJisx0212.c     \
                 aciConvKsc5601.c      \
                 aciConvSjis.c         \
                 aciConvCp932.c        \
                 aciConvCp932ext.c     \
                 aciConvUhc1.c         \
                 aciConvUhc2.c         \
                 aciConvUtf8.c         \
                 aciConvBig5.c         \
                 aciConvCp936.c        \
                 aciConvCp936ext.c     \
                 aciConvGbk.c          \
                 aciConvGbkext1.c      \
                 aciConvGbkext2.c      \
                 aciConvGbkextinv.c    \
                 aciVarString.c

GENMSG_GSRCS   = genmsgParse.c genmsgLex.c
GENMSG_SRCS    = $(GENMSG_GSRCS) genmsgCode.c genmsgManual.c genmsg.c

LIBEDIT_DIR=$(CORE_DIR)/acl/libedit
EXTERNAL_LIB_DIR=$(CORE_EXT_DIR)

ifeq ($(READLINE_SUPPORT),yes)
LIBEDIT_SRCS =      \
        chared.c    \
        common.c    \
        el.c        \
        emacs.c     \
        fcns.c      \
        fgetln.c    \
        help.c      \
        hist.c      \
        history.c   \
        key.c       \
        map.c       \
        parse.c     \
        prompt.c    \
        read.c      \
        refresh.c   \
        search.c    \
        sig.c       \
        strlcat.c   \
        strlcpy.c   \
        term.c      \
        tokenizer.c \
        tty.c       \
        unvis.c     \
        vi.c        \
        vis.c       \
        readline.c  \
        local_ctype.c
else
LIBEDIT_SRCS =
endif

EXTERNAL_LIB_SRCS = \
                lz4.c

ZOOKEEPER_DIR=$(CORE_EXT_DIR)/src/zookeeper-client

ZOOKEEPER_SRCS =       \
     zookeeper.c       \
     recordio.c        \
     zookeeper.jute.c  \
     zk_log.c          \
     zk_hashtable.c    \
     addrvec.c         \
     mt_adaptor.c      \
     hashtable_itr.c   \
     hashtable.c

LIBEDIT_OBJS   = $(LIBEDIT_SRCS:.c=$(OBJ_SUF))
LIBEDIT_SHOBJS = $(LIBEDIT_SRCS:.c=$(SHOBJ_SUF))

EXTERNAL_LIB_OBJS   = $(EXTERNAL_LIB_SRCS:.c=$(OBJ_SUF))
EXTERNAL_LIB_SHOBJS = $(EXTERNAL_LIB_SRCS:.c=$(SHOBJ_SUF))

ZOOKEEPER_OBJS   = $(ZOOKEEPER_SRCS:.c=$(OBJ_SUF))
ZOOKEEPER_SHOBJS = $(ZOOKEEPER_SRCS:.c=$(SHOBJ_SUF))

CORE_OBJS      = $(ACP_SRCS:.c=$(OBJ_SUF))       \
                 $(ACP_GASM_SRCS:.gs=$(OBJ_SUF)) \
                 $(ACP_ASM_SRCS:.s=$(OBJ_SUF))   \
                 $(ACE_SRCS:.c=$(OBJ_SUF))       \
                 $(ACL_SRCS:.c=$(OBJ_SUF))       \
                 $(ACI_SRCS:.c=$(OBJ_SUF))       \
                 $(LIBEDIT_OBJS)                 \
                 $(EXTERNAL_LIB_OBJS)

CORE_SHOBJS    = $(ACP_SRCS:.c=$(SHOBJ_SUF))       \
                 $(ACP_GASM_SRCS:.gs=$(SHOBJ_SUF)) \
                 $(ACP_ASM_SRCS:.s=$(SHOBJ_SUF))   \
                 $(ACE_SRCS:.c=$(SHOBJ_SUF))       \
                 $(ACL_SRCS:.c=$(SHOBJ_SUF))       \
                 $(ACI_SRCS:.c=$(SHOBJ_SUF))       \
                 $(LIBEDIT_SHOBJS)                 \
                 $(EXTERNAL_LIB_SHOBJS)

ifeq ($(ALTI_CFG_OS),LINUX)
CORE_OBJS     += $(ZOOKEEPER_OBJS)
CORE_SHOBJS   += $(ZOOKEEPER_SHOBJS)
endif

# COMPILER, COMPILE_BIT are from root.mk
ifneq ($(COMPILER),gcc)
 ifeq ($(ALTI_CFG_OS),SOLARIS)
  # SunOS, SPARC CPU
  ifeq ($(ALTI_CFG_CPU),SPARC)
   ifeq ($(COMPILE_BIT),32)
    CORE_OBJS   += $(CORE_DIR)/acp/objects/acpAtomic-SPARC_CAS_32MODE.o
    CORE_SHOBJS += $(CORE_DIR)/acp/objects/acpAtomic-SPARC_CAS_32MODE_pic.o
    CORE_OBJS   += $(CORE_DIR)/acp/objects/acpMemBarrier-SPARC_32MODE.o
    CORE_SHOBJS += $(CORE_DIR)/acp/objects/acpMemBarrier-SPARC_32MODE_pic.o
   else
    CORE_OBJS   += $(CORE_DIR)/acp/objects/acpAtomic-SPARC_CAS_64MODE.o
    CORE_SHOBJS += $(CORE_DIR)/acp/objects/acpAtomic-SPARC_CAS_64MODE_pic.o
    CORE_OBJS   += $(CORE_DIR)/acp/objects/acpMemBarrier-SPARC_64MODE.o
    CORE_SHOBJS += $(CORE_DIR)/acp/objects/acpMemBarrier-SPARC_64MODE_pic.o
  endif
  # SunOS, X86 CPU
  else ifeq ($(ALTI_CFG_CPU),X86)
   ifeq ($(COMPILE_BIT),32)
    CORE_OBJS   += $(CORE_DIR)/acp/objects/acpAtomic-X86_SOLARIS_32MODE.o
    CORE_SHOBJS += $(CORE_DIR)/acp/objects/acpAtomic-X86_SOLARIS_32MODE_pic.o
    CORE_OBJS   += $(CORE_DIR)/acp/objects/acpMemBarrier-X86_SOLARIS_32MODE.o
    CORE_SHOBJS += $(CORE_DIR)/acp/objects/acpMemBarrier-X86_SOLARIS_32MODE_pic.o
   else
    CORE_OBJS   += $(CORE_DIR)/acp/objects/acpAtomic-X86_SOLARIS_64MODE.o
    CORE_SHOBJS += $(CORE_DIR)/acp/objects/acpAtomic-X86_SOLARIS_64MODE_pic.o
    CORE_OBJS   += $(CORE_DIR)/acp/objects/acpMemBarrier-X86_SOLARIS_64MODE.o
    CORE_SHOBJS += $(CORE_DIR)/acp/objects/acpMemBarrier-X86_SOLARIS_64MODE_pic.o
   endif
  endif
 else ifeq ($(ALTI_CFG_OS),HPUX)
  # HPUX, IA64 CPU
  ifeq ($(ALTI_CFG_CPU),IA64)
   ifeq ($(COMPILE_BIT),32)
    CORE_OBJS   += $(CORE_DIR)/acp/objects/acpAtomic-HPUX_IA64_CAS16_32MODE.o
    CORE_SHOBJS += $(CORE_DIR)/acp/objects/acpAtomic-HPUX_IA64_CAS16_32MODE_pic.o
    CORE_OBJS   += $(CORE_DIR)/acp/objects/acpAtomic-HPUX_IA64_32MODE.o
    CORE_SHOBJS += $(CORE_DIR)/acp/objects/acpAtomic-HPUX_IA64_32MODE_pic.o
    CORE_OBJS   += $(CORE_DIR)/acp/objects/acpMemBarrier-HPUX_IA64_32MODE.o
    CORE_SHOBJS += $(CORE_DIR)/acp/objects/acpMemBarrier-HPUX_IA64_32MODE_pic.o
   else
    CORE_OBJS   += $(CORE_DIR)/acp/objects/acpAtomic-HPUX_IA64_CAS16_64MODE.o
    CORE_SHOBJS += $(CORE_DIR)/acp/objects/acpAtomic-HPUX_IA64_CAS16_64MODE_pic.o
    CORE_OBJS   += $(CORE_DIR)/acp/objects/acpAtomic-HPUX_IA64_64MODE.o
    CORE_SHOBJS += $(CORE_DIR)/acp/objects/acpAtomic-HPUX_IA64_64MODE_pic.o
    CORE_OBJS   += $(CORE_DIR)/acp/objects/acpMemBarrier-HPUX_IA64_64MODE.o
    CORE_SHOBJS += $(CORE_DIR)/acp/objects/acpMemBarrier-HPUX_IA64_64MODE_pic.o
   endif
  # HPUX, PARISC CPU
  else ifeq ($(ALTI_CFG_CPU),PARISC)
   ifeq ($(COMPILE_BIT),32)
    CORE_OBJS   += $(CORE_DIR)/acp/objects/acpSpinLock-PARISC_32MODE.o
    CORE_SHOBJS += $(CORE_DIR)/acp/objects/acpSpinLock-PARISC_32MODE_pic.o
    CORE_OBJS   += $(CORE_DIR)/acp/objects/acpMemBarrier-PARISC_32MODE.o
    CORE_SHOBJS += $(CORE_DIR)/acp/objects/acpMemBarrier-PARISC_32MODE_pic.o
   else
    CORE_OBJS   += $(CORE_DIR)/acp/objects/acpSpinLock-PARISC_64MODE.o
    CORE_SHOBJS += $(CORE_DIR)/acp/objects/acpSpinLock-PARISC_64MODE_pic.o
    CORE_OBJS   += $(CORE_DIR)/acp/objects/acpMemBarrier-PARISC_64MODE.o
    CORE_SHOBJS += $(CORE_DIR)/acp/objects/acpMemBarrier-PARISC_64MODE_pic.o
   endif # end of COMPILE_BIT
  endif # end of ALTI_CFG_CPU
 else ifeq ($(ALTI_CFG_OS),AIX)
  ifeq ($(ALTI_CFG_CPU),POWERPC)
   ifeq ($(COMPILE_BIT),32)
    CORE_OBJS   += $(CORE_DIR)/acp/objects/acpAtomic-PPC64_32MODE.o
    CORE_SHOBJS += $(CORE_DIR)/acp/objects/acpAtomic-PPC64_32MODE_pic.o
   endif # end of COMPILE_BIT
  endif # end of ALTI_CFG_CPU
 endif # end of ALTI_CFG_OS
endif # end of COMPILER

CTEST_OBJS     = $(ACT_SRCS:.c=$(OBJ_SUF))
CTEST_SHOBJS   = $(ACT_SRCS:.c=$(SHOBJ_SUF))

CORE_SRCDIRS   = acp ace acl acl/libedit/src acl/externalLib/src acl/externalLib/src/zookeeper-client aci
CTEST_SRCDIRS  = act

CORE_OBJDIRS   = $(addprefix $(CORE_BUILD_DIR)/,$(CORE_SRCDIRS))
CTEST_OBJDIRS  = $(addprefix $(CORE_BUILD_DIR)/,$(CTEST_SRCDIRS))

CORE_LIBS      = core
CTEST_LIBS     = ctest

ALTICORE_LIBS      = alticore_$(ALTICORE_VERSION)
ALTICTEST_LIBS     = altictest_$(ALTICORE_VERSION)

ALTICORE_PICLIBS      = alticore_pic_$(ALTICORE_VERSION)
ALTICTEST_PICLIBS     = altictest_pic_$(ALTICORE_VERSION)

GENMSG         = $(BINS_DIR)/genmsg$(EXEC_SUF)

CORE_INCLUDES  = $(INC_OPT)$(CORE_DIR)/include
CORE_INCLUDES += $(INC_OPT)$(CORE_EXT_DIR)/include

CORE_LIBDIRS   = $(CORE_BUILD_DIR)/lib

INCLUDES      += $(CORE_INCLUDES)
INCLUDES      += $(PLATFORM_EXTERNAL_INCLUDES)
LIBDIRS       += $(LIBS_DIR)

ALINT_SILENCES    += aclCompression.h aclLZ4.h lz4.h readline.h aciCallback.h aciConvEuckr.h aciConvSjis.h aciMsgLog.h aciConv.h aciConvGb2312.h aciConvUhc1.h aciProperty.h aciConvAscii.h aciConvJisx0201.h aciConvUhc2.h aciTypes.h aciConvBig5.h aciConvJisx0208.h aciConvUtf8.h aciVa.h aciConvCharSet.h aciConvJisx0212.h aciErrorMgr.h aciVarString.h aciConvCp949.h aciConvKsc5601.h aciHashUtil.h aciVersion.h aciConvEucjp.h aciConvReplaceCharMap.h aciMsg.h
