# $Id: Makefile 70913 2015-05-21 09:03:59Z sungminee $

include ../mtEnv.mk

SRCS = $(MTL_SRCS)

OBJS=$(SRCS:$(DEV_DIR)/%.cpp=$(TARGET_DIR)/%.$(OBJEXT))

all: $(COMPILE)

shobjs: $(TARGET_DIR)/src/mt/mtl/mtl_shobj.$(OBJEXT) $(TARGET_DIR)/src/mt/mtl/mtlUTF8ForClient_shobj.$(OBJEXT) $(TARGET_DIR)/src/mt/mtl/mtlASCIIForClient_shobj.$(OBJEXT) $(TARGET_DIR)/src/mt/mtl/mtlKSC5601ForClient_shobj.$(OBJEXT) $(TARGET_DIR)/src/mt/mtl/mtlShiftJISForClient_shobj.$(OBJEXT) $(TARGET_DIR)/src/mt/mtl/mtlGB231280ForClient_shobj.$(OBJEXT) $(TARGET_DIR)/src/mt/mtl/mtlBig5ForClient_shobj.$(OBJEXT) $(TARGET_DIR)/src/mt/mtl/mtlMS949ForClient_shobj.$(OBJEXT) $(TARGET_DIR)/src/mt/mtl/mtlCollate_shobj.$(OBJEXT)


compile: $(OBJS)

clean:
	$(RM) $(TARGET_DIR)/src/mt/mtl *~ .shobj

-include $(OBJS:.$(OBJEXT)=.d)
