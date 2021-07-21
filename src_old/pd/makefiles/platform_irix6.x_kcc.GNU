# platform_irix6.x_kcc.GNU,v 4.3 1999/06/05 22:04:17 coryan Exp

# Irix 6.x with KCC 3.3f

include $(PD_DIR)/makefiles/platform_irix6.x_common.GNU

# WARNING_FLAGS +=
# 111: statement is unreachable.    There's a whole bunch of these
#                                   in the pdl code.
WARNING_FLAGS += --diag_suppress 111

debug = 1
optimize = 1

CC              = KCC
CXX             = KCC
CFLAGS          += -32
CCFLAGS         += $(CFLAGS) -DNULL=0 $(WARNING_FLAGS)
ifeq ($(optimize),1)
DCFLAGS         += -g --backend -g3
else
DCFLAGS         += +K0 -g
endif
CDLD            = $(CC)
DLD             = $(CXX)
LD              = $(CXX)
LDFLAGS         += -32 -L$(PD_DIR)/pdl -L.
LIBS            += -lpthread
OCFLAGS         += +K3 -O2
PIC             = 
AR              = KCC
ARFLAGS         = -o
RANLIB          = @true
SOFLAGS         = $(CPPFLAGS)
SOBUILD         = $(COMPILE.cc) $(PIC) -o $(VSHDIR)$*.so $<
PRELIB          = @true
