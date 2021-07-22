# platform_cray.GNU,v 4.2 1999/07/20 02:19:05 levine Exp

# Platform macros for SGI/Cray UNICOS (vector) & UNICOS/mk (mpp) -
#    UNICOS 9.0.2.8
#    UNICOS 10.0.0.5
#    UNICOS/mk 2.0.4.57
#    Cray C++ Version 3.2.0.1
#    Cray Standard C Version 6.2.0.1
#
# Contributed by Doug Anderson <dla@home.com>

debug = 1
exceptions = 1
static_libs_only = 1
quantify =
purify =

# Cray's autoinstantiation seems to be broken, resulting in
# C++ prelinker: error: instantiation loop
# So, we're using PDL_HAS_EXPLICIT_TEMPLATE_INSTANTIATION
CC              = cc
LDLIB           += -L$(PD_DIR)/pdl
CXX             = CC
#CFLAGS         +=
CCFLAGS         += -h noautoinstantiate
DCFLAGS         += -g
DCCFLAGS        += $(DCFLAGS)
OCFLAGS         += -O2
OCCFLAGS        += $(OCFLAGS) $(added_optimizer_flag)

ifndef exceptions
  CCFLAGS += -h noexceptions
else
  CCFLAGS += -h exceptions
endif # ! exceptions

CDLD            = $(CC)
DLD             = $(CXX)
LD              = $(CXX)
LIBS            = -lpthread
PIC             =
AR              = ar
ARFLAGS         = ruv
RANLIB          = /bin/true

# This is necessary to get libraries to contain template info, but only
# necessary when using autoinstantiation
# PRELIB     = @echo ""; \
#  echo "int main() { return 0; }" >dummy.cpp; \
#  echo "$(CXX) $(CCFLAGS) $(CPPFLAGS) $(LDFLAGS) $(LIBS) -o dummy dummy.cpp $?";\
#  $(CXX) $(CCFLAGS) $(CPPFLAGS) $(LDFLAGS) $(LIBS) -o dummy dummy.cpp $?;
