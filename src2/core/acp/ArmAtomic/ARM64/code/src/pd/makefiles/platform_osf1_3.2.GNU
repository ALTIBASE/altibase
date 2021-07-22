# platform_osf1_3.2.GNU,v 4.12 1998/10/09 16:59:06 levine Exp

# This platform macros file is intended to work with Digital UNIX 3.2
# (OSF/1 3.2g) with CXX 5.7 or later.

debug = 1

CC              = cxx
CXX             = $(CC)
CFLAGS          += -threads
#### CCFLAGS         += -x cxx
CCFLAGS         += $(CFLAGS) -w0
DCFLAGS         += -g -O0
#### DLD             = ld -shared -expect_unresolved '*'
DLD             = ld
LD              = $(CXX)
#### LIBS            += -lpthreads -lmach -lsys5 -lxti -ltli -lcxx -lexc -lc -lrt
LIBS            += -qltli_r -ltli -qlrt_r -lrt -qlcxxstd_r -lcxxstd -qlcxx_r \
                   -lcxx -qlots_r -lots -lpthreads -lmach -lexc -lc_r -lc
#### PIC             = -pic
ARFLAGS         = cruv
RANLIB          = /usr/bin/true
#### SOFLAGS         = -shared
SOFLAGS         = -L/usr/lib/cmplrs/cxx -rpath /usr/lib/cmplrs/cxx \
                  -L$(PD_DIR)/pdl -L./ -g2 -O0 -shared
SOBUILD         = $(RM) $@; $(COMPILE.cc) $(PIC) -o $(VSHDIR)$*.o $< && \
                  ln $(VSHDIR)$*.o $@
