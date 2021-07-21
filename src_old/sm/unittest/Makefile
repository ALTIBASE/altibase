####################################################
#   Unit Test Makefile
#
#   Usage :
#       Just add the name of Unit Test Souce Code
####################################################

BLD_DIR = sm/unittest

include $(ALTIDEV_HOME)/unittest.mk

UNITTEST_TARGETS = smSample \
				   smCompareLSN \
				   smSdpInit

smSample_NAME = unittestSmSample
smSample_SRCS = unittestSmSample.cpp
smSample_SHLIBS = alticore altictest

smCompareLSN_NAME = unittestSmrCompareLSN
smCompareLSN_SRCS = unittestSmrCompareLSN.cpp
smCompareLSN_SHLIBS = alticore altictest sm_so id_so pd_so

smSdpInit_NAME = unittestSdpInitialize
smSdpInit_SRCS = unittestSdpInitialize.cpp
smSdpInit_SHLIBS = alticore altictest sm_so id_so pd_so

# Run unittests one by one.
.NOTPARALLEL: unittest_content

include $(MAKEFILES_DIR)/rule.mk
