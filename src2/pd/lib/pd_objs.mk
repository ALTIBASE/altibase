# Makefile for PD library
#
# CVS Info : $Id: pd_objs.mk 68602 2015-01-23 00:13:11Z sbjang $
#

# NORMAL LIBRARY
PDL_SRCS   =   $(PD_DIR)/pdl/Basic_Types.cpp  \
               $(PD_DIR)/pdl/OS.cpp           \
               $(PD_DIR)/pdl/PDL2.cpp         \
               $(PD_DIR)/pdl/Handle_Set2.cpp

ifeq "$(CONFIG_LACKS_IOSTREAM)" "y"
PDL_SRCS += $(DEV_DIR)/src/pd/port/iostream/istream.cpp
endif

PD_SRCS    =   $(PDL_SRCS)
PD_OBJS  = $(PD_SRCS:$(DEV_DIR)/%.cpp=$(TARGET_DIR)/%.$(OBJEXT))
PD_SHOBJS  =   $(PD_SRCS:$(DEV_DIR)/%.cpp=$(TARGET_DIR)/%_shobj.$(OBJEXT))
PD_CLIENT_SRCS = $(PD_SRCS)
PD_CLIENT_OBJS = $(PD_CLIENT_SRCS:$(DEV_DIR)/%.cpp=$(TARGET_DIR)/%.$(OBJEXT))
PD_CLIENT_SHOBJS = $(PD_CLIENT_SRCS:$(DEV_DIR)/%.cpp=$(TARGET_DIR)/%_shobj.$(OBJEXT))
