# platform_cygwin32.GNU,v 4.1 2001/04/21 22:43:23 coryan Exp

#
# This file should allow to build PDL for cygwin32 with cygwin tools.
#
# Edit to change TCPU and w32api defs below.
#
# Don't forget to define the PD_DIR environment variable!
#

#
# Chose your target CPU (uncomment ONE of these).
#
#TCPU = pentiumpro
#TCPU = pentium
#TCPU = i486
#TCPU = i386
TCPU = i686

cygwin32 = 1
#threads = 0
static_libs_only = 1
optimize = 0

include $(PD_DIR)/makefiles/platform_gnuwin32_common.GNU
