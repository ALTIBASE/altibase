include $(ALTI_HOME)/install/altibase_env.mk

BINS=split

all: $(BINS)

split:
	$(LD) $(LFLAGS) $(GOPT) $(INCLUDES) -o split split.cpp

clean2:
	-rm -rf split split.o

