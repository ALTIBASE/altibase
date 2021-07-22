# platform_irix6.x_common.GNU,v 4.1 1999/06/05 22:04:17 coryan Exp

ifndef PDL_IRIX_VERS
PDL_IRIX_VERS       := $(subst .,,$(shell uname -r))
endif # PDL_IRIX_VERS

CPPFLAGS += -DPDL_IRIX_VERS=$(PDL_IRIX_VERS)
