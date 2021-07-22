####################################################
#   Unit Test Makefile
#
#   Usage :
#       Just add the name of Unit Test Souce Code
####################################################

BLD_DIR = rp/unittest

include $(ALTIDEV_HOME)/unittest.mk

UNITTEST_TARGETS =	rpSample	\
					rpdQueue	\
					rprSNMapMgr

rpSample_NAME = unittestRpSample
rpSample_SRCS = unittestRpSample.cpp
rpSample_SHLIBS = alticore altictest id_so pd_so

rpdQueue_NAME = unittestRpdQueue
rpdQueue_SRCS = unittestRpdQueue.cpp
rpdQueue_SHLIBS = alticore altictest mt_so cm_so qp_so sm_so rp_so id_so pd_so

rprSNMapMgr_NAME = unittestRprSNMapMgr
rprSNMapMgr_SRCS = unittestRprSNMapMgr.cpp
rprSNMapMgr_SHLIBS = alticore altictest mt_so cm_so qp_so sm_so rp_so id_so pd_so st_so rc_so

# Run unittests one by one.
.NOTPARALLEL: unittest_content

include $(MAKEFILES_DIR)/rule.mk
