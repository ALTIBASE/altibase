# $Id: Makefile 16585 2006-06-08 11:33:43Z shsuh $
#

include ../idEnv.mk


SRCS=$(IDKS_SRCS)
OBJS=$(SRCS:.S=.$(OBJEXT))

INCLUDES+=$(IDROPT)$(ID_DIR)/idk

all:ofiles

ofiles:$(OBJS)

alint: $(SRCS:.c=.lint)

clean:
	$(RM) $(TARGET_DIR)/src/id/idk core .dependency *~

-include $(OBJS:.$(OBJEXT)=.d)
