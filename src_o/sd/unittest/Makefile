####################################################
#   Unit Test Makefile
#
#   Usage :
#       Just add the name of Unit Test Souce Code
####################################################

BLD_DIR = sd/unittest

include $(ALTIDEV_HOME)/unittest.mk

UNITTEST_TARGETS = sdSample

sdSample_NAME = unittesdSdSample
sdSample_SRCS = unittesdSdSample.cpp
sdSample_LIBS = id pd
sdSample_SHLIBS = alticore altictest

# Run unittests one by one.
.NOTPARALLEL: unittest_content

include $(MAKEFILES_DIR)/rule.mk
