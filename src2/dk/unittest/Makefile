####################################################
#   Unit Test Makefile
#
#   Usage :
#       Just add the name of Unit Test Souce Code
####################################################

BLD_DIR = dk/unittest

include $(ALTIDEV_HOME)/unittest.mk

UNITTEST_TARGETS = dkSample

dkSample_NAME = unittestDkSample
dkSample_SRCS = unittestDkSample.cpp
dkSample_SHLIBS = alticore altictest

# Run unittests one by one.
.NOTPARALLEL: unittest_content

include $(MAKEFILES_DIR)/rule.mk
