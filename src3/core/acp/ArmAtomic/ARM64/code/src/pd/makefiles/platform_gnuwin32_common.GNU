# platform_gnuwin32_common.GNU,v 4.3 2002/04/07 09:38:05 kitty Exp

ifneq ($(mingw32),1)
  ifneq ($(cygwin32),1)
    fail := \
      $(shell \
          echo "You should not use this file but one of"; \
          echo "platform_mingw32.GNU or platform_cygwin32.GNU"; \
          exit 1)
  endif
endif

# debug = 1
# optimize = 1
# exceptions = 0

ifndef winsock2
  winsock2 = 1
endif

ifndef exceptions
  exceptions = 0
endif
ifeq (,$(debug))
  debug = 1
endif
ifeq (,$(optimize))
  optimize = 1
endif
ifeq (,$(threads))
  threads = 1
endif
ifeq ($(threads),0)
  CFLAGS += -DPDL_MT_SAFE=0
endif # threads

ifeq ($(mingw32),1)
CFLAGS += \
	-I$(MINGW_BASE)/mingw32/include \
	-I$(MINGW_BASE)/w32api/include \
	-I$(MINGW_BASE)/include
LDFLAGS += \
	-L$(MINGW_BASE)/mingw32/lib \
	-L$(MINGW_BASE)/w32api/lib \
	-L$(MINGW_BASE)/lib
endif

ifeq ($(debug),0)
CFLAGS += -DNDEBUG
endif

CC      = gcc
CXX     = g++

MODEL_FLAGS += -m$(TCPU)

# Never use -pipe with 95/98. On NT/2000, not sure if it helps.
CFLAGS  += -W -Wall -Wpointer-arith
ifeq ($(threads),1)
        MODEL_FLAGS += -mthreads
endif # threads

CFLAGS += $(MODEL_FLAGS)

# Never use -pipe with 95/98. On NT/2000, not sure if it helps.
CFLAGS  += -W -Wall -Wpointer-arith

# Test for template instantiation.
#
include $(PD_DIR)/makefiles/platform_g++_common.GNU

EXEEXT = .exe

CCFLAGS += $(CFLAGS) $(TEMPLATES_FLAG)
DCFLAGS += -g
DLD     = $(CXX)
LD      = $(CXX)

ifeq ($(mingw32),1)
ifeq ($(winsock2),1)
LIBS    += -lws2_32 -lmswsock
endif
LIBS    += -lwsock32
endif

OCFLAGS += -O3
PIC        = -DPDL_OS_HAS_DLL=1
AR         = ar
ARFLAGS    = rsuv
RANLIB     = ranlib
SOEXT      = dll
SOFLAGS    = $(MODEL_FLAGS) $(CPPFLAGS)
SOBUILD    = $(COMPILE.cc) $(PIC) -o $(VSHDIR)$*.so $<

# The sed below is to strip comments on the .def file,
# to workaround to a bug in ld.
SHLIBBUILD = \
  $(RM) $(VSHDIR)/$@.def.old $(VSHDIR)/$@.def; \
  dlltool --export-all --output-def $(VSHDIR)/$@.def --dllname $@ \
    $(VSHOBJS1) \
  && mv $(VSHDIR)/$@.def $(VSHDIR)/$@.def.old \
  && sed 's/;.*$$//g' < $(VSHDIR)/$@.def.old > $(VSHDIR)/$@.def \
  && $(SOLINK.cc) -Wl,--enable-auto-image-base -Wl,--out-implib,$@.a \
    -shared -o $@ $(LDFLAGS) $(VSHDIR)/$@.def \
    $(VSHOBJS1) $(PDL_SHLIBS) $(LIBS)

ifeq ($(exceptions),1)
SHLIBBUILD += \
 ;echo "Warning: exceptions fail when mixing DLL and non DLL calls."
endif

PRELIB  = @true
