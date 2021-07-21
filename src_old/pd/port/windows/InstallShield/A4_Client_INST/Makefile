include $(ALTI_HOME)/install/altibase_env.mk

SRCS=odbcsetup.cpp
OBJS=$(SRCS:.cpp=.$(OBJEXT))
OUT=odbcsetup$(BINEXT)

all: build

build: $(OBJS)
	$(LD) $(OBJS) $(LFLAGS) $(LDOUT)$(OUT) $(LIBS)

clean:
	rm -f $(OUT)
	rm -f $(OBJS)
	rm -f *.res

-include $(OBJS:.$(OBJEXT)=.d)
