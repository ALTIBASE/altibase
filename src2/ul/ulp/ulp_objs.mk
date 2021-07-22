ULP_GEN_SRCS= $(UL_DIR)/ulp/ulpPreprocl.cpp \
              $(UL_DIR)/ulp/ulpPreprocy.cpp   \
              $(UL_DIR)/ulp/ulpPreprocifl.cpp \
              $(UL_DIR)/ulp/ulpPreprocify.cpp \
              $(UL_DIR)/ulp/ulpCompl.cpp  \
              $(UL_DIR)/ulp/ulpCompy.cpp

ULP_BIN_SRCS= $(UL_DIR)/ulp/ulpMain.cpp                \
              $(UL_DIR)/ulp/ulpProgOption.cpp          \
              $(UL_DIR)/ulp/ulpGenCode.cpp             \
              $(UL_DIR)/ulp/ulpHash.cpp                \
              $(UL_DIR)/ulp/ulpSymTable.cpp            \
              $(UL_DIR)/ulp/ulpStructTable.cpp         \
              $(UL_DIR)/ulp/ulpScopeTable.cpp          \
              $(UL_DIR)/ulp/ulpMacroTable.cpp          \
              $(UL_DIR)/ulp/ulpIfStackMgr.cpp          \
              $(UL_DIR)/ulp/ulpErrorMgr.cpp

ULP_LIB_SRCS = $(UL_DIR)/ulp/lib/ulpLibErrorMgr.c     \
               $(UL_DIR)/ulp/lib/ulpLibMultiErrorMgr.c \
               $(UL_DIR)/ulp/lib/ulpLibHash.c         \
               $(UL_DIR)/ulp/lib/ulpLibConnection.c   \
               $(UL_DIR)/ulp/lib/ulpLibStmtCur.c      \
               $(UL_DIR)/ulp/lib/ulpLibError.c        \
               $(UL_DIR)/ulp/lib/ulpLibInterFuncA.c    \
               $(UL_DIR)/ulp/lib/ulpLibInterFuncB.c    \
               $(UL_DIR)/ulp/lib/ulpLibInterCoreFuncA.c    \
               $(UL_DIR)/ulp/lib/ulpLibInterCoreFuncB.c    \
               $(UL_DIR)/ulp/lib/ulpLibInterface.c    \
               $(UL_DIR)/ulp/lib/ulpLibOption.c       
