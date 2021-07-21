CMB_SRCS        = $(CM_DIR)/cmb/cmbBlock.cpp                \
                  $(CM_DIR)/cmb/cmbPool.cpp                 \
                  $(CM_DIR)/cmb/cmbPoolLOCAL.cpp            \
                  $(CM_DIR)/cmb/cmbPoolIPC.cpp              \
                  $(CM_DIR)/cmb/cmbShm.cpp                  \
                  $(CM_DIR)/cmb/cmbShmIPCDA.cpp             \
                  $(CM_DIR)/cmb/cmbPoolIPCDA.cpp

CMB_CLIENT_SRCS = $(CM_DIR)/cmb/cmbBlock.c                  \
                  $(CM_DIR)/cmb/cmbPool.c                   \
                  $(CM_DIR)/cmb/cmbPoolLOCAL.c              \
                  $(CM_DIR)/cmb/cmbPoolIPC.c                \
                  $(CM_DIR)/cmb/cmbShm.c                    \
                  $(CM_DIR)/cmb/cmbShmIPCDA.c               \
                  $(CM_DIR)/cmb/cmbPoolIPCDA.c

CMI_SRCS        = $(CM_DIR)/cmi/cmi.cpp

CMI_CLIENT_SRCS = $(CM_DIR)/cmi/cmi.c


CMM_SRCS        = $(CM_DIR)/cmm/cmmSession.cpp

CMM_CLIENT_SRCS = $(CM_DIR)/cmm/cmmSession.c


CMN_SRCS        = $(CM_DIR)/cmn/cmnSock.cpp                     \
                  $(CM_DIR)/cmn/cmnSockIB.cpp                   \
                  $(CM_DIR)/cmn/cmnLink.cpp                     \
                  $(CM_DIR)/cmn/cmnOpenssl.cpp                  \
                  $(CM_DIR)/cmn/cmnIB.c                         \
                  $(CM_DIR)/cmn/cmnLinkListenTCP.cpp            \
                  $(CM_DIR)/cmn/cmnLinkListenSSL.cpp            \
                  $(CM_DIR)/cmn/cmnLinkListenUNIX.cpp           \
                  $(CM_DIR)/cmn/cmnLinkListenIPC.cpp            \
                  $(CM_DIR)/cmn/cmnLinkListenIPCDA.cpp          \
                  $(CM_DIR)/cmn/cmnLinkListenIB.cpp             \
                  $(CM_DIR)/cmn/cmnLinkPeer.cpp                 \
                  $(CM_DIR)/cmn/cmnLinkPeerDUMMY.cpp            \
                  $(CM_DIR)/cmn/cmnLinkPeerTCP.cpp              \
                  $(CM_DIR)/cmn/cmnLinkPeerSSL.cpp              \
                  $(CM_DIR)/cmn/cmnLinkPeerUNIX.cpp             \
                  $(CM_DIR)/cmn/cmnLinkPeerIPC.cpp              \
                  $(CM_DIR)/cmn/cmnLinkPeerIPCDA.cpp            \
                  $(CM_DIR)/cmn/cmnLinkPeerIB.cpp               \
                  $(CM_DIR)/cmn/cmnDispatcher.cpp               \
                  $(CM_DIR)/cmn/cmnDispatcherIPC.cpp            \
                  $(CM_DIR)/cmn/cmnDispatcherIPCDA.cpp          \
                  $(CM_DIR)/cmn/cmnDispatcherSOCK-SELECT.cpp    \
                  $(CM_DIR)/cmn/cmnDispatcherSOCK-POLL.cpp      \
                  $(CM_DIR)/cmn/cmnDispatcherSOCK-EPOLL.cpp     \
                  $(CM_DIR)/cmn/cmnDispatcherIB.cpp


CMN_CLIENT_SRCS = $(CM_DIR)/cmn/cmnSock.c                   \
                  $(CM_DIR)/cmn/cmnSockIB.c                 \
                  $(CM_DIR)/cmn/cmnLink.c                   \
                  $(CM_DIR)/cmn/cmnOpenssl.c                \
                  $(CM_DIR)/cmn/cmnIB.c                     \
                  $(CM_DIR)/cmn/cmnLinkListenTCP.c          \
                  $(CM_DIR)/cmn/cmnLinkListenUNIX.c         \
                  $(CM_DIR)/cmn/cmnLinkListenIB.c           \
                  $(CM_DIR)/cmn/cmnLinkPeer.c               \
                  $(CM_DIR)/cmn/cmnLinkPeerTCP.c            \
                  $(CM_DIR)/cmn/cmnLinkPeerSSL.c            \
                  $(CM_DIR)/cmn/cmnLinkPeerUNIX.c           \
                  $(CM_DIR)/cmn/cmnLinkPeerIPC.c            \
                  $(CM_DIR)/cmn/cmnLinkPeerIPCDA.c          \
                  $(CM_DIR)/cmn/cmnLinkPeerIB.c             \
                  $(CM_DIR)/cmn/cmnDispatcher.c             \
                  $(CM_DIR)/cmn/cmnDispatcherIPC.c          \
                  $(CM_DIR)/cmn/cmnDispatcherIPCDA.c        \
                  $(CM_DIR)/cmn/cmnDispatcherSOCK.c         \
                  $(CM_DIR)/cmn/cmnDispatcherIB.c


CMP_SRCS        = $(CM_DIR)/cmp/cmpHeader.cpp               \
                  $(CM_DIR)/cmp/cmpCallbackBASE.cpp         \
                  $(CM_DIR)/cmp/cmpArgBASE.cpp              \
                  $(CM_DIR)/cmp/cmpArgDB.cpp                \
                  $(CM_DIR)/cmp/cmpArgRP.cpp                \
                  $(CM_DIR)/cmp/cmpArgDK.cpp                \
                  $(CM_DIR)/cmp/cmpMarshalBASE.cpp          \
                  $(CM_DIR)/cmp/cmpMarshalDB.cpp            \
                  $(CM_DIR)/cmp/cmpMarshalRP.cpp            \
                  $(CM_DIR)/cmp/cmpMarshalDK.cpp            \
                  $(CM_DIR)/cmp/cmpModuleBASE.cpp           \
                  $(CM_DIR)/cmp/cmpModuleDB.cpp             \
                  $(CM_DIR)/cmp/cmpModuleRP.cpp             \
                  $(CM_DIR)/cmp/cmpModuleDK.cpp             \
                  $(CM_DIR)/cmp/cmpModuleDR.cpp             \
                  $(CM_DIR)/cmp/cmpModule.cpp

CMP_CLIENT_SRCS = $(CM_DIR)/cmp/cmpHeader.c                 \
                  $(CM_DIR)/cmp/cmpCallbackBASE.c           \
                  $(CM_DIR)/cmp/cmpArgBASE.c                \
                  $(CM_DIR)/cmp/cmpArgDB.c                  \
                  $(CM_DIR)/cmp/cmpArgRP.c                  \
                  $(CM_DIR)/cmp/cmpMarshalBASE.c            \
                  $(CM_DIR)/cmp/cmpMarshalDB.c              \
                  $(CM_DIR)/cmp/cmpMarshalRP.c              \
                  $(CM_DIR)/cmp/cmpModuleBASE.c             \
                  $(CM_DIR)/cmp/cmpModuleDB.c               \
                  $(CM_DIR)/cmp/cmpModuleRP.c               \
                  $(CM_DIR)/cmp/cmpModule.c


CMT_SRCS        = $(CM_DIR)/cmt/cmtAny.cpp                  \
                  $(CM_DIR)/cmt/cmtVariable.cpp             \
                  $(CM_DIR)/cmt/cmtCollection.cpp

CMT_CLIENT_SRCS = $(CM_DIR)/cmt/cmtAny.c                    \
                  $(CM_DIR)/cmt/cmtVariable.c               \
                  $(CM_DIR)/cmt/cmtCollection.c


CMU_SRCS        = $(CM_DIR)/cmu/cmuTraceCode.cpp            \
                  $(CM_DIR)/cmu/cmuIpcPath.cpp              \
                  $(CM_DIR)/cmu/cmuIPCDAPath.cpp            \
                  $(CM_DIR)/cmu/cmuProperty.cpp

CMU_CLIENT_SRCS = $(CM_DIR)/cmu/cmuVersion.c                \
                  $(CM_DIR)/cmu/cmuTraceCode.c              \
                  $(CM_DIR)/cmu/cmuIPCDAPath.c              \
                  $(CM_DIR)/cmu/cmuIpcPath.c


CMX_SRCS        = $(CM_DIR)/cmx/cmxXid.cpp

CMX_CLIENT_SRCS = $(CM_DIR)/cmx/cmxXid.c


CM_SRCS         = $(CMB_SRCS) $(CMI_SRCS) $(CMM_SRCS) $(CMN_SRCS) $(CMP_SRCS) $(CMT_SRCS) $(CMU_SRCS) $(CMX_SRCS)

CM_CLIEINT_SRCS = $(CMB_CLIENT_SRCS) $(CMI_CLIENT_SRCS) $(CMM_CLIENT_SRCS) $(CMN_CLIENT_SRCS) $(CMP_CLIENT_SRCS) $(CMT_CLIENT_SRCS) $(CMU_CLIENT_SRCS) $(CMX_CLIENT_SRCS)

CM_OBJS         = $(patsubst $(DEV_DIR)/%, $(TARGET_DIR)/%.$(OBJEXT),       $(basename $(CM_SRCS)))
CM_SHOBJS       = $(patsubst $(DEV_DIR)/%, $(TARGET_DIR)/%_shobj.$(OBJEXT), $(basename $(CM_SRCS)))
