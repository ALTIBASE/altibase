include $(ALTI_HOME)/install/altibase_env.mk

SRCS=altimon.cpp
OBJS=$(SRCS:$(DEV_DIR)/%.cpp=$(TARGET_DIR)/%.$(OBJEXT))

BINS=altimon
	 
all: $(BINS)

altimon: altimon.$(OBJEXT)
	$(LD) $(LFLAGS) $(LDOUT)altimon$(BINEXT) altimon.$(OBJEXT) $(LIBOPT)odbccli$(LIBAFT) $(LIBS)

clean:
	-$(RM) $(BINS) *.$(OBJEXT) core

-include $(OBJS:.$(OBJEXT)=.d)
