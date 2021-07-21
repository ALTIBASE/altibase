# $Id: Makefile 26804 2008-07-01 02:06:37Z kimmkeun $

include ../mtEnv.mk

SRCS = $(MTCL_SRCS)

OBJS=$(SRCS:$(DEV_DIR)/%.c=$(TARGET_DIR)/%.$(OBJEXT))

all: $(COMPILE)

shobjs: $(TARGET_DIR)/src/mt/mtcl/mtl_shobj.$(OBJEXT) $(TARGET_DIR)/src/mt/mtcl/mtlUTF8ForClient_shobj.$(OBJEXT) $(TARGET_DIR)/src/mt/mtcl/mtlASCIIForClient_shobj.$(OBJEXT) $(TARGET_DIR)/src/mt/mtcl/mtlKSC5601ForClient_shobj.$(OBJEXT) $(TARGET_DIR)/src/mt/mtcl/mtlShiftJISForClient_shobj.$(OBJEXT) $(TARGET_DIR)/src/mt/mtcl/mtlGB231280ForClient_shobj.$(OBJEXT) $(TARGET_DIR)/src/mt/mtcl/mtlBig5ForClient_shobj.$(OBJEXT) $(TARGET_DIR)/src/mt/mtcl/mtlMS949ForClient_shobj.$(OBJEXT) $(TARGET_DIR)/src/mt/mtcl/mtlCollate_shobj.$(OBJEXT)


compile: $(OBJS)

clean:
	$(RM) $(TARGET_DIR)/src/mt/mtcl *~ .shobj

-include $(OBJS:.$(OBJEXT)=.d)
