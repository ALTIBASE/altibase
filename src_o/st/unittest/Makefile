####################################################
#   Unit Test Makefile
#
#   Usage :
#       Just add the name of Unit Test Souce Code
####################################################

BLD_DIR = st/unittest

include $(ALTIDEV_HOME)/unittest.mk

UNITTEST_TARGETS = stSample

stSample_NAME = unittestStSample
stSample_SRCS = unittestStSample.cpp
stSample_LIBS = id pd
stSample_SHLIBS = alticore altictest

# Run unittests one by one.
.NOTPARALLEL: unittest_content

include $(MAKEFILES_DIR)/rule.mk
