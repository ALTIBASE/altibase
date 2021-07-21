# platform_tru64_g++.GNU,v 4.1 1999/07/20 02:19:05 levine Exp

# This platform macros file is intended to work with
# Digital UNIX using the GNU compiler.

ifndef PDL_DU_VERSION
  PDL_DU_VERSION := $(shell uname -r)
endif # PDL_DU_VERSION

ifeq (3.,$(findstring 3.,$(PDL_DU_VERSION)))
  include $(PD_DIR)/makefiles/platform_osf1_3.2.GNU

else # ! 3.x
ifeq (4.,$(findstring 4.,$(PDL_DU_VERSION)))
  include $(PD_DIR)/makefiles/platform_osf1_4.0_g++.GNU

  ifeq (4.0F,$(findstring 4.0F,$(shell /usr/sbin/sizer -v)))
    CFLAGS += -DDIGITAL_UNIX=0x40F
  else  # ! 4.0F
    CFLAGS += -DDIGITAL_UNIX=0x400
  endif # ! 4.0F

else # ! 4.x
ifeq (5.,$(findstring 5.,$(PDL_DU_VERSION)))
  include $(PD_DIR)/makefiles/platform_osf1_4.0_g++.GNU
  CFLAGS += -DDIGITAL_UNIX=0x500

endif # 5.x
endif # ! 4.x
endif # ! 3.x
