# $Id: Makefile 74314 2016-01-28 04:09:32Z timmy.kim $

include ../mtEnv.mk

SRCS = $(MTD_SRCS)
OBJS=$(SRCS:$(DEV_DIR)/%.cpp=$(TARGET_DIR)/%.$(OBJEXT))

GENSRCS=$(MT_DIR)/mtd/mtddl.cpp
GENLEXSRCS=$(MT_DIR)/mtd/mtddlex.l
GENOBJS=$(GENSRCS:$(DEV_DIR)/%.cpp=$(TARGET_DIR)/%.$(OBJEXT))
GENHEADER=mtddlLexer.h

LEX=$(FLEX)
LEXFLAGS=-R -Cfar

all: compile

shobjs: $(VSHOBJS)

compile: $(GENOBJS) $(OBJS)

$(GENOBJS): $(GENSRCS)

SERVER_LIBS=-lid -lpd -xnolib -Bdynamic -lthread -lposix4 -ldl -lkvm -lkstat -lsocket -lnsl -lgen  -lm -lw -lc -Bstatic -liostream -lCrun -lst

genflex :
	$(LEX)  $(LEXFLAGS)  -o$(GENSRCS) $(GENLEXSRCS)
	$(COPY) $(GENHEADER) $(MT_DIR)/include/$(GENHEADER)
	$(RM) $(GENHEADER)

genclean:
	$(RM) $(GENOBJS) $(GENSRCS) $(MT_DIR)/include/$(GENHEADER) *~ .shobj

clean:
	$(RM) $(TARGET_DIR)/src/mt/mtd *~ .shobj

-include $(OBJS:.$(OBJEXT)=.d)
