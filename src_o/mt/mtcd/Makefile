# $Id: Makefile 26804 2008-07-01 02:06:37Z kimmkeun $

include ../mtEnv.mk

SRCS = $(MTCD_SRCS)
OBJS=$(SRCS:$(DEV_DIR)/%.c=$(TARGET_DIR)/%.$(OBJEXT))

GENSRCS=$(MT_DIR)/mtcd/mtcddl.c
GENLEXSRCS=$(MT_DIR)/mtcd/mtcddlex.l
GENOBJS=$(GENSRCS:$(DEV_DIR)/%.c=$(TARGET_DIR)/%.$(OBJEXT))
GENHEADER=mtcddlLexer.h

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
	$(RM) $(TARGET_DIR)/src/mt/mtcd *~ .shobj

-include $(OBJS:.$(OBJEXT)=.d)
