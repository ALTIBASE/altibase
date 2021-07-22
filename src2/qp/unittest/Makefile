####################################################
#   Unit Test Makefile
#
#   Usage :
#       Just add the name of Unit Test Souce Code
####################################################

BLD_DIR = qp/unittest

include $(ALTIDEV_HOME)/unittest.mk

UNITTEST_TARGETS = qpSample unittestQtcDependency unittestQcmFixedTable

qpSample_NAME = unittestQpSample
qpSample_SRCS = unittestQpSample.cpp
qpSample_SHLIBS = alticore altictest

unittestQtcDependency_NAME = unittestQtcDependency
unittestQtcDependency_SRCS = unittestQtcDependency.cpp
unittestQtcDependency_SHLIBS = alticore altictest qp_so rp_so mt_so sm_so cm_so id_so pd_so st_so rc_so

unittestQcmFixedTable_NAME = unittestQcmFixedTable
unittestQcmFixedTable_SRCS = unittestQcmFixedTable.cpp
unittestQcmFixedTable_SHLIBS = alticore altictest qp_so rp_so mt_so sm_so cm_so id_so pd_so st_so rc_so

# Run unittests one by one.
.NOTPARALLEL: unittest_content

include $(MAKEFILES_DIR)/rule.mk

