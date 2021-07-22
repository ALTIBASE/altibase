# platform_tru64_cxx.GNU,v 1.11 1999/07/28 02:06:42 levine Exp

# This platform macros file is intended to work with
# Digital UNIX using the cxx compiler.

ifndef PDL_DU_VERSION
  PDL_DU_VERSION := $(shell uname -r)
endif # ! PDL_DU_VERSION

ifeq (3.,$(findstring 3.,$(PDL_DU_VERSION)))
  include $(PD_DIR)/makefiles/platform_osf1_3.2.GNU
  CFLAGS += -ieee_with_no_inexact 

else # ! 3.x
ifeq (4.,$(findstring 4.,$(PDL_DU_VERSION)))
  include $(PD_DIR)/makefiles/platform_osf1_4.0.GNU
  CFLAGS += -ieee_with_no_inexact 

  ifndef PDL_DU_SUBVERSION
    PDL_DU_SUBVERSION := $(shell /usr/sbin/sizer -v)
  endif # ! PDL_DU_SUBVERSION

  ifeq (4.0F,$(findstring 4.0F,$(PDL_DU_SUBVERSION)))
    CFLAGS += -DDIGITAL_UNIX=0x40F
  else
  ifeq (4.0E,$(findstring 4.0E,$(PDL_DU_SUBVERSION)))
    CFLAGS += -DDIGITAL_UNIX=0x40E
  else
  ifeq (4.0D,$(findstring 4.0D,$(PDL_DU_SUBVERSION)))
    CFLAGS += -DDIGITAL_UNIX=0x40D
  else
    CFLAGS += -DDIGITAL_UNIX=0x400
  endif
  endif
  endif

  ifneq ($(CXX_VER),CXX_5)
    # 9: nested comment not allowed.  But there's one in /usr/include/pdsc.h!
    WARNING_FLAGS += -msg_disable 9
  endif # 6.0 or later

else # ! 4.x
ifeq (5.,$(findstring 5.,$(PDL_DU_VERSION)))
  include $(PD_DIR)/makefiles/platform_osf1_4.0.GNU
  CFLAGS += -DDIGITAL_UNIX=0x500
  CFLAGS += -ieee_with_no_inexact 
  SOFLAGS = -shared -use_ld_input

endif # 5.x
endif # ! 4.x
endif # ! 3.x
