#
# $Id$
#

include ../sdEnv.mk

ifneq "$(OS_LINUX_PACKAGE)" "ARM_WINCE"
INCLUDES += $(foreach i,/usr/local/include $(QP_DIR)/include $(SD_DIR)/include,$(IDROPT)$(i))
LIBDIRS  += $(foreach i,$(SM_DIR)/lib $(SD_DIR)/lib,$(LDROPT)$(i))
else # ARM_WINCE
INCLUDES += $(foreach i,/usr/local/include $(QP_DIR)/include $(SD_DIR)/include $(DEV_DIR)/win32-build/iostream,$(IDROPT)$(i))
LIBDIRS  += $(foreach i,$(SM_DIR)/lib $(SD_DIR)/lib $(DEV_DIR)/win32-build/iostream,$(LDROPT)$(i))
endif # ARM_WINCE

GENSRCS= $(SD_DIR)/sdp/sdpjl.cpp $(SD_DIR)/sdp/sdpjy.cpp
GENOBJS=$(GENSRCS:$(DEV_DIR)/%.cpp=$(TARGET_DIR)/%.$(OBJEXT))
GENSTUFFS=sdpjy.cpp.output sdpjy.cpp.h

SRCS= $(SD_DIR)/sdp/sdpjManager.cpp
OBJS=$(SRCS:$(DEV_DIR)/%.cpp=$(TARGET_DIR)/%.$(OBJEXT))

LEX=$(FLEX)
ifeq "$(CONFIG_QP_COMPRESS_SCANNER_TABLES)" "y"
LEXFLAGS=-Car
else
LEXFLAGS=-Cfar
endif

YACC=$(BISON)
YACCFLAGS=-d -t -v


%.cpp: %.l
	$(LEX)  $(LEXFLAGS)  -o$*.cpp $*.l
	$(SED) s,$(FLEX_REPLACE_BEFORE),$(FLEX_REPLACE_AFTER), < $*.cpp > $*.cpp.old
	$(RM) -f $*.cpp
	mv $*.cpp.old $*.cpp

all: compile iostream

iostream:
ifeq "$(OS_LINUX_PACKAGE)" "ARM_WINCE"
	$(MAKE) -C $(DEV_DIR)/win32-build/iostream
endif # ARM_WINCE

compile: $(GENOBJS) $(OBJS)

header :
	@for i in $(SRCS); do ( cp $$i $$i.org ; cat $$i.org | gawk  -f $(TOOL_DIR)/add_header/header.awk > $$i ) done

check :
	@for i in $(SRCS); do (gawk -v API_INPUT=$(TOOL_DIR)/coding_chk/API.txt -v SRC_INPUT=$$i -f $(TOOL_DIR)/coding_chk/apiCheck.awk ) done

alint: $(SRCS:.cpp=.lint)

clean:
	$(RM) core .dependency $(GENOBJS) $(OBJS) *.d SunWS_cache $(GENSTUFFS) $(GENSRCS)

$(GENOBJS): $(GENSRCS)

##################################################################
#  특정 플랫폼(windows)에서는 Shell의 environement로서
#  BISON_SIMPLE, BISON_HAIRY 가 정의되어 있어야 한다.
#  기존에는 batch file을 이용하였으나, 복잡, 이식성이 떨어지므로,
#  GNU Makefile의 인자로서 환경변수를 설정하는 기능을 이용한다. 아싸, 심플!
##################################################################

%.cpp: %.y
ifeq "$(BISON_ENV_NEEDED)" "yes"
	$(MAKE) $@ BISON_SIMPLE=$(BISON_SIMPLE_PATH) BISON_HAIRY=$(BISON_HAIRY_PATH) BISON_ENV_NEEDED=no
else
	$(YACC) $(YACCFLAGS) -p $(subst y.cpp,,$(notdir $@)) -o $*.cpp $*.y
endif

alint: $(SRCS:.cpp=.lint)

-include $(OBJS:.$(OBJEXT)=.d)
-include $(GENOBJS:.$(OBJEXT)=.d)
