# platform_hpux_aCC.GNU,v 4.25 1999/07/21 16:35:59 shuston Exp
#
# This is for the HP-UX 10.x and 11.x using the aC++ compiler.
#
# For 10.x, most of the difference for threaded vs. non-threaded is
# contained in pdl/config-hpux-10.x.h.  However, to build without threads,
# you should run the make with "threads=0" since the default is to build
# with threads.
#
# HP-UX 11 supports both 32-bit and 64-bit builds, regardless of which
# architecture the build is done on.  This file is set up to build native
# to the machine it's running on.  To cross-build for another platform, add
# "buildbits=32" to make a 32-bit build, and "buildbits=64" to do a 64-bit
# build.  Note that if you explicitly specify a 64-bit build, the option
#  -Wl,+vnocompatwarnings is added to shut up the "At least one PA 2.0 object
# file detected..." messages.

# by gamestar : 일관성있는 매크로 사용을 위함 : use compile64
#ifeq (,$(buildbits))
#  buildbits = 0
#endif
ifeq (,$(debug))
  debug = 1
endif
ifeq (,$(distrib))
  distrib = 0
endif
ifeq (,$(optimize))
  optimize = 0
endif
ifeq (,$(threads))
  threads = 1
endif

# modified by gamestar 
buildbits = 32
ifeq ($(compile64),1)
  buildbits = 64
else
  buildbits = 32
endif

# First, extract the OS version number.
HPVERS_WORDS := $(subst ., ,$(shell uname -r))
HPUX_VERS	:= $(join $(word 2,$(HPVERS_WORDS)),$(word 3,$(HPVERS_WORDS)))
#
CC		= /bin/cc
CXX		= /opt/aCC/bin/aCC
#
# Set the appropriate preprocessor defs for threading based on OS version.
# If the user has requested building without threads, then don't set any
# for 10.20, but for 11.00, the default is to use threads, so explicitly
# turn them off if the user requested build without threads.
#
ifeq ($(threads),1)
ifeq ($(word 2,$(HPVERS_WORDS)), 11)
THR_DEFS	= -DPDL_HAS_THREADS -D_POSIX_C_SOURCE=199506L
else
THR_DEFS	= -DPDL_HAS_THREADS -D_REENTRANT
endif
else
ifeq ($(word 2,$(HPVERS_WORDS)), 11)
THR_DEFS	= -DPDL_HAS_THREADS=0
endif
endif

# Suppress warning 302 ((...) parameter list is a non-portable feature)
CCFLAGS		+= $(CFLAGS) $(THR_DEFS) -DHPUX_VERS=$(HPUX_VERS) -DPDL_LACKS_PRAGMA_ONCE +W302 -D_HPUX_SOURCE -D_XOPEN_SOURCE -D_XOPEN_SOURCE_EXTENDED=1
# Additionally, on HP-UX 10.20, suppress 495 to shut up the warnings from
# the system header files.  667 is also suppressed, but the compiler still
# tells you there was a future error, but at least you can pick out any real
# errors by quickly scanning the output. 829 is suppressed because the
# system headers have offending string literals assigned to char *.
ifeq ($(word 2,$(HPVERS_WORDS)), 10)
CCFLAGS         += +W495,667,829
endif
DCFLAGS		+= +ESdbgasm -g -DDEBUG
CDLD            = $(CC)
DLD		= $(CXX) $(CCFLAGS)
LD		= $(CXX)

# because of PR-2119(compiler bug at HP 11.11) by jdlee
ifeq ($(OS_MINORVER), 11)
OCFLAGS		= +O2 +Ofltacc +Onolimit +Oentrysched $(added_optimizer_flag)
OCCFLAGS	= +O2 +Ofltacc +Onolimit +Oentrysched $(added_optimizer_flag)
else
ifeq ($(OS_MINORVER), 23)
OCFLAGS		= +O2 +Ofltacc $(added_optimizer_flag)
OCCFLAGS	= +O2 +Ofltacc $(added_optimizer_flag)
else
OCFLAGS		= +O2 +Ofltacc $(added_optimizer_flag)
OCCFLAGS	= +O2 +Ofltacc $(added_optimizer_flag)
endif
endif

PIC		= +Z

AR		= $(COMPILE.cc) +inst_close $^; /bin/ar
ARFLAGS		= ruv
RANLIB		= echo
LDFLAGS		= -Wl,+s -z
SOFLAGS		= -b

ifeq ($(distrib),1)

ifeq ($(word 2,$(HPVERS_WORDS)), 10)
SOFLAGS		+= -Wl,-x -Wl,+h/usr/lib/$(@F)
else
ifeq ($(buildbits),32)
SOFLAGS		+= -Wl,-x -Wl,+h/usr/lib/$(@F)
endif
ifeq ($(buildbits),64)
SOFLAGS		+= -Wl,-x -Wl,+h/usr/lib/pa20_64/$(@F)
endif
endif

endif

SOEXT		= sl

ifeq ($(buildbits),32)
	ifeq ($(OS_TARGET),IA64_HP_HPUX) 
# BUG-21932 : IA64_HP_HPUX에서 32bit 빌드시 발생하는 bus error를 막기위해 +u4를 추가한다.
		CFLAGS		= -AP +DA1.1 +DS1.1 -mt +u4
	else
		CFLAGS		= +DA1.1 +DS1.1 -mt
	endif
else
    ifeq ($(buildbits),64)
        ifeq ($(OS_MINORVER),22)
            CFLAGS = -AP +DD64
        else
            ifeq ($(OS_MINORVER),23)
                ifeq ($(OS_TARGET),IA64_HP_HPUX)
                    CFLAGS = -AP +DD64
                else
                    CFLAGS = +DD64
                endif
            else
                ifeq ($(OS_TARGET),IA64_HP_HPUX)
                    CFLAGS = -AP +DD64
                else
                    CFLAGS = +DA2.0W +DS2.0
                endif
            endif
        endif

        LDFLAGS += -Wl,+vnocompatwarnings
    else
        CFLAGS   = +DAportable
    endif
endif

ifndef exceptions
  CCFLAGS += +noeh
endif # ! exceptions


#
# Grab the appropriate libraries based on the version of HP-UX we're building
# on and whether or not thread support is being used.
#
ifeq ($(word 2,$(HPVERS_WORDS)), 11)

#11.x:
ifeq ($(threads),1)
LIBS		+= -lxti -lpthread -lrt -ldld
else
LIBS		+= -lxti -lrt -ldld
endif

else

# 10.x:
LIBS		+= -lxti -ldld
ifeq ($(threads),1)
LIBS		+= -lcma
endif

endif
