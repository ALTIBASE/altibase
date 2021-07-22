include $(ALTI_HOME)/install/altibase_env.mk

all: dbStatus

%.c : %.sc
	sesc $<

dbStatus: dbStatus.$(OBJEXT) dbStatus.c
	$(LD) $(LFLAGS) $(GOPT) $(INCLUDES) -o dbStatus dbStatus.$(OBJEXT) -lsesc -lodbccli -lst $(LIBS)

clean:
	-$(RM) *.$(OBJEXT) *.c core *.class

