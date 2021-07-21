# platform_sunos5_sunc++.GNU,v 4.54 1999/12/06 14:26:32 levine Exp

# SunOS 5.x (Solaris 2.x) with Sun C++ 4.2 and 5.x, *not* using Orbix.
#
# NOTE: Sun C++ 5.0 users might need to add this to their CCFLAGS:
#   -compat=4 and/or remove -instances=explicit.  Please note that
#   inlining is disabled by default with Sun C++ 5.0.  If you'd like
#   to try enabling it, comment out the "inline = 0" line below.
#
# To enable compilation of 64-bit binaries with Sun CC 5.0, a
# platform_macros.GNU file such as this can be used:
#     fast=1
#     include $(PD_DIR)/makefiles/platform_sunos5_sunc++.GNU
#     CCFLAGS += -xtarget=ultra2 -xarch=v9a #### CPU specific!
#     SOFLAGS += -xtarget=ultra2 -xarch=v9a
#     LDFLAGS += -L/opt/SUNWspro/SC5.0/lib/v9 -L/usr/lib/sparcv9
# Please note that the xtarget and xarch settings are specific to
# the target CPU.  The /opt/SUNWspro/ Sun C++ installation directory
# is site-specific.
#
# With slight modification, this file could be used with Sun C++ 4.1.
# However, it's likely that you won't be able to build all of PDL
# with Sun C++ 4.1.
#
# NOTE: some PDL files might generate this warning:
#   "Warning: Could not find source for " one of the following:
#    default constructor, copy constructor, destructor, or assignment
#    operator.  It appears that with +w, Sun C++ 4.x issues this warning
#    whenever it has to generate one of these functions (because it wasn't
#    defined by the user).  This isn't really a problem.
#    This warning appears to be fixed by Sun C++ 4.2 jumbo patch 104631-02.
#
# -pta instantiates all template members, which makes libraries bigger.
# But, it's supposed to put each member into it's own .o, so executable
# size should not be penalized.  We don't use it because it's not
# necessary with automatic template instantiation.

ifeq ($(threads),1)
  CFLAGS += -mt
  LDFLAGS += -mt
endif # threads

CC              = cc
CXX             = CC

CC_VERSION = $(shell CC -V 2>&1)

ifeq (Compilers 4.2,$(findstring Compilers 4.2,$(CC_VERSION)))
  #### CC 4.2, only, supports -features=castop
  CCFLAGS      += $(CFLAGS) -features=castop

  #### RTTI can coexist with PDL, but it doesn't use it.  To enable,
  #### add rtti=1 to your make invocation.  Sun C++ 4.2, only, supports
  #### this option.
  ifdef rtti
    CCFLAGS += -features=rtti
  else  # ! rtti
    #### PDL_LACKS_RTTI is necessary with -features=castop, but
    #### without -features=rtti, to convert dynamic_cast to static_cast.
    CCFLAGS += -DPDL_LACKS_RTTI
  endif # ! rtti

  #### Create template repository to avoid compiler warning.
  TEMPLATE_REPOSITORY = Templates.DB
else  # ! 4.2
ifeq (Compilers 5,$(findstring Compilers 5,$(CC_VERSION)))
  #### CC 5.0 or later
  #### Inlining appears to cause link problems with early releases of
  #### CC 5.0.

#  by sjkim : erase this line for inline enable
#  inline = 0

  #### If compat=4 is desired, set up for that. There are syntactic diffs
  #### in the compatibility modes, but here all we need to be concerned with
  #### is turning on -compat=4, and not trying to enable the new libraries.
  #### RTTI may work, but has not been tested. To try it, cut/paste the
  #### rtti conditions from the 4.2 section, above.
  ifeq ($(compat4),1)
    CCFLAGS += -compat=4 $(CFLAGS) -DPDL_LACKS_RTTI -instances=explicit -D_POSIX_PTHREAD_SEMANTICS -D_REENTRANT
  else
    #### Support RTTI by default.  And, use explicit template instantiation
    #### because automatic instantiation appears to be broken, at least
    #### with early releases of CC 5.0.  And, use old iostreams.
    CCFLAGS += +w $(CFLAGS) -library=iostream,no%Cstd -instances=explicit -xildoff -D_POSIX_PTHREAD_SEMANTICS -D_REENTRANT -DPDL_LACKS_RTTI
  endif # compat4 requested

else
ifeq (C++ 5,$(findstring C++ 5,$(CC_VERSION)))
  #### CC 5.5
  #### Inlining appears to cause link problems with early releases of
  #### CC 5.0.

  #### If compat=4 is desired, set up for that. There are syntactic diffs
  #### in the compatibility modes, but here all we need to be concerned with
  #### is turning on -compat=4, and not trying to enable the new libraries.
  #### RTTI may work, but has not been tested. To try it, cut/paste the
  #### rtti conditions from the 4.2 section, above.
  ifeq ($(compat4),1)
    CCFLAGS += -compat=4 $(CFLAGS) -DPDL_LACKS_RTTI -instances=explicit -D_POSIX_PTHREAD_SEMANTICS -D_REENTRANT
  else
    #### Support RTTI by default.  And, use explicit template instantiation
    #### because automatic instantiation appears to be broken, at least
    #### with early releases of CC 5.0.  And, use old iostreams.
    ifeq (C++ 5.7,$(findstring C++ 5.7,$(CC_VERSION)))
      CCFLAGS += $(CFLAGS) -library=iostream,no%Cstd -instances=explicit -xildoff -D_POSIX_PTHREAD_SEMANTICS -D_REENTRANT -DPDL_LACKS_RTTI
    else
      CCFLAGS += $(CFLAGS) -library=iostream,no%Cstd -instances=explicit -xildoff -D_POSIX_PTHREAD_SEMANTICS -D_REENTRANT -DPDL_LACKS_RTTI
    endif # CC 5.7
  endif # compat4 requested


endif # CC 5.5
endif # CC 5.0 or later
endif # ! 4.2

DCFLAGS         += -g -DDEBUG
CDLD            = $(CC)
DLD             = $(CXX)


#### Exception handling breaks on MP machines prior to Sun C++ 4.2
#### with jumbo patch 104631-02, see pdl/config-sunos5.5.h.
#### If you are using a version of Sun C++ prior to 4,2, or haven't
#### applied that jumbo patch to your 4.2, or don't want to pay the
#### performance penalty of exception handling, then use the default
#### of no exceptions.  If you want to enable exception handling,
#### add "exceptions=1" to your "make" invocation.
ifndef exceptions
  CCFLAGS += -noex
endif # ! exceptions

LD              = $(CXX)

ifeq ($(distrib),0)
  LDFLAGS       += -R $(PD_DIR)/pdl -R./
endif

LIBS          += -lsocket -ldl -lnsl -lgen -lposix4

ifeq (Compilers 5,$(findstring Compilers 5,$(CC_VERSION)))
        ifeq ($(compat4),1)
               COMPAT_LIBDIR=/opt/SUNWspro/lib/CC4
               LIBS    += -L$(COMPAT_LIBDIR) -lC
        endif
endif

LDFLAGS         += -xildoff

ifndef fast # by jdlee
OCFLAGS         += -O $(added_optimizer_flag)
else
OCFLAGS         += $(added_optimizer_flag)
endif #fast

PIC             = -KPIC
AR              = CC
ARFLAGS         = -xar -o
RANLIB          = echo
#SOFLAGS         = -G $(CCFLAGS)
SOFLAGS         = -G $(CFLAGS)  ## by jdlee for odbc_perl

ifdef BIN
  #### Builds .shobj/$*.o file, then .shobj/$*.so file.  The .o files
  #### are used for building libraries and executables.  But, it seems
  #### to be necessary to build the .so files (with -G) in order to get
  #### all template instantiations.
  SOBUILD = $(COMPILE.cc) $(PIC) -o $(VSHDIR)$*.o $<; \
            $(SOLINK.cc) -o $@ -h $@ $(LDFLAGS) $(VSHDIR)$*.o
else
  #### Optimize builds when no executables are built in the current
  #### directory.  Only a library is being created, and -G is used
  #### in that step.  Therefore, all templates instantations are
  #### included in the library.  This optimization saves almost 11 Mb
  #### (2.6 percent) and 27 minutes (22 percent) on the entire PDL
  #### build on a 168 MHz Sun Ultra2.
  ####
  #### Sun C++ won't allow the output from a compile to be named with
  #### a .so extension.  Rather than muck with the PDL build rules and
  #### risk upsetting builds on other platforms, just ln the output
  #### after building it.
  SOBUILD = $(RM) $@; $(COMPILE.cc) $(PIC) -o $(VSHDIR)$*.o $< && \
                      /bin/ln $(VSHDIR)$*.o $@
endif # BIN

#### The following macro overrides enable creation of fast executables.
#### They _don't_ support fast compilation :-)
#### To use, add fast=1 to your "make" invocation.
####
#### -g is incompatible with -fast.  If you need -g, you can
#### use "-fast -O3".
####
ifdef fast
ifeq ($(compile64),1)
    CFLAGS += -fast -fsimple=0 $(added_optimizer_flag)
    DCFLAGS =
    LDFLAGS += -fast 
else
    CFLAGS += -fast -fsimple=0 $(added_optimizer_flag)
    DCFLAGS =
    LDFLAGS += -fast
endif # compile64
endif # fast

#### because of cc compiler bug, -xarch option should take a precedence
#### then -fast
ifeq ($(compile64),1)
    ASFLAGS += -xarch=amd64
    CFLAGS += -xarch=amd64
    LDFLAGS += -xarch=amd64 -L/opt/SUNWspro/lib/amd64 -L/usr/lib/amd64
else
#   CFLAGS += -xarch=generic
    LDFLAGS += -L/opt/SUNWspro/lib
endif

ifdef SB
	CCFLAGS += -xsb
endif
