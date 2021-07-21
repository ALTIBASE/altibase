# platform_tru64_pre.GNU,v 4.1 2001/08/08 19:22:38 kitty Exp

# Common definitions for all versions of Digital Unix.

ifndef PDL_DU_VERSION
  PDL_DU_VERSION := $(shell uname -r)
endif # ! PDL_DU_VERSION

ifndef PDL_DU_SUBVERSION
  PDL_DU_SUBVERSION := $(shell /usr/sbin/sizer -v)
endif # ! PDL_DU_SUBVERSION
