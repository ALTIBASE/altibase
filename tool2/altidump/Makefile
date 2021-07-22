include ../toolEnv.mk

BIN=$(ALTI_HOME)/bin/altidump$(BINEXT)# sample sample_thr
all : $(BIN)

ifeq "$(OS_TARGET)" "INTEL_WINDOWS"
# ------------------------------------------------
#   WINDOWS ENV
# ------------------------------------------------

LIBS		:= $(LIBOPT)id$(LIBAFT) $(LIBOPT)pd$(LIBAFT) $(LIBS)
LIBDIRS		+= $(LDROPT)$(ALTI_HOME)/lib $(LDROPT)"C:\Program Files\Debugging Tools for Windows"
INCLUDES	:= $(IDROPT). $(IDROPT)$(ID_DIR)/include $(INCLUDES)
CC = cl.exe

.PHONY: altidump
altidump: $(ALTI_HOME)/bin/altidump$(BINEXT)

$(ALTI_HOME)/bin/altidump$(BINEXT): $(TARGET_DIR)/tool/altidump/altidump-win.$(OBJEXT)
	$(LD) $(LFLAGS)  $(LDOUT)$@ $<  $(LIBDIRS) $(LIBS)  /MAP:dump.map

else #unix env.
# ------------------------------------------------
#   UNIX ENV
# ------------------------------------------------

# if this link make error, add -lintl(on AIX?) or remove -lintl
# because of the difference between several binutil versions.

INCLUDES := $(IDROPT)/usr/local/include $(IDROPT)$(ID_DIR)/include $(IDROPT). $(INCLUDES)
ifeq "$(OS_TARGET)" "X86_64_DARWIN"
#LIB=#$(LDROPT)/opt/sfw/lib #$(LDROPT)/usr/local/lib #
LIBDIR += $(LDROPT)$(ID_DIR)/lib $(LDROPT)$(PD_DIR)/lib $(LDROPT)/usr/local/lib -L/opt/local/lib -L/opt/local/lib/x86_64
else
LIB=#$(LDROPT)/opt/sfw/lib #$(LDROPT)/usr/local/lib #
LIBDIR=$(LDROPT)$(ID_DIR)/lib $(LDROPT)$(PD_DIR)/lib $(LDROPT)/usr/local/lib 
endif
GNULIBS=-lbfd -liberty 

ifeq "$(OS_TARGET)" "IBM_AIX"
GNULIBS := $(GNULIBS) -lintl -lz
LIBDIR=$(LDROPT)$(ID_DIR)/lib $(LDROPT)$(PD_DIR)/lib $(LDROPT)/usr/lib
endif

ifeq "$(OS_TARGET)" "POWERPC64_LINUX"
LIBDIR=$(LDROPT)$(ID_DIR)/lib $(LDROPT)$(PD_DIR)/lib $(LDROPT)/usr/lib64
endif


ifeq "$(OS_TARGET)" "DEC_TRU64"
GNULIBS := $(GNULIBS) -lintl 
endif

CC = gcc
CFLAGS = -g

ifeq "$(OS_TARGET)" "X86_64_DARWIN"
CFLAGS += -flat_namespace
endif

$(ALTI_HOME)/bin/altidump$(BINEXT): altidump.$(OBJEXT)
	$(CC) $(CFLAGS) $(CCOUT)$@ $< $(LIBDIR) $(LIB) $(GNULIBS)

altidump.$(OBJEXT) : altidump.c
ifeq "$(OS_TARGET)" "X86_64_DARWIN"
	$(CC) $(INCLUDES) $(CFLAGS) $(LIBDIR) -c $<
else
	$(CC) $(INCLUDES) $(CFLAGS) -c $<
endif
endif

clean:
	$(RM) *.log *.$(OBJEXT) $(BIN) $(SAMPLE_OBJS) core
