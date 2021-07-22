# Makefile with Genernal Options
#
# CVS Info : $Id: ibm_aix_vac.mk 88304 2020-08-07 06:18:48Z kclee $
#

# 전달되는 외부 변수들 : GCC

# ID_DIR      : SM 디렉토리 
# ID_ACE_ROOT : 라이브러리 패스
# compile64   : 컴파일 환경
# compat5     : CC 5.0 으로?

ifndef	BUILD_MODE
	@echo "ERROR BUILD_MODE!!!!"
	@exit
endif

.SUFFIXES: .cpp .$(OBJEXT) .a

CXX	= xlC_r
CC	= xlc_r
LD	= $(CXX)

AR	= ar

NM 	= /usr/bin/nm

PURIFY		= purify -chain-length=100
QUANTIFY	= quantify
PURECOV		= purecov
PURIFYCOV   = $(PURIFY) $(PURECOV)

# IDL(ACE) Library
# Library

# readline library 설정

ifeq "$(USE_READLINE)" "1"
READLINE_INCLUDES = -I/usr/local/include/readline
READLINE_LIBRARY =  -lreadline -ltermcap 
endif # use readline library 


ifeq "$(OS_MAJORVER)" "5"
ifeq "$(USE_HEAPMIN)" "no"
LIBS           = $(READLINE_LIBRARY) -lpthreads
else
LIBS           = $(READLINE_LIBRARY) -lpthreads -lhm
endif

LIBS_SHIP	=
else
LIBS        = $(READLINE_LIBRARY) -lpthreads
LIBS_SHIP	=
endif # "$(OS_MAJORVER)" "5"

# 동적 컴파일 옵션 선언
#
CXXOPT_DEPENDANCY = -MM

# BUG-31647
ifeq ($(compile64),1)
else
    LIBS          += -lgcc_s
endif

# Build Mode file for Makefile 
#
# CVS Info : $Id: ibm_aix_vac.mk 88304 2020-08-07 06:18:48Z kclee $
#

# BUILD_MODE의 종류
#	debug		: Debug 모드
#   prerelease      : -DDEBUG(x) -g (o)
#	release		: release 버젼, 실제 product에 해당

# LINK MODE 종류 
#	purify		: purify version
#	quantify	: quantify version
#	purecov		: purecov version
#	purifycov   : purifycov version


ifeq "$(BUILD_MODE)" "debug"
LFLAGS	+= -g
else
ifeq "$(BUILD_MODE)" "prerelease"
LFLAGS	+= -g
else
ifeq "$(BUILD_MODE)" "release"
#LFLAGS	+= -O3 -funroll-loops ## link error - by jdlee
LFLAGS	+= -O2 -qinline
else
error:
	@echo "ERROR!!!! UNKNOWN BUILD_MODE($(BUILD_MODE))";
	@exit;
endif	# release
endif   # prerelease
endif	# debug

############### LINK MODE #####################################3
ifeq "$(LINK_MODE)" "normal"
LD		:=  $(LD)
else
ifeq "$(LINK_MODE)" "purify"
  ifeq "$(OS_TARGET)" "HP_HPUX"
    LD		:= $(PURIFY) -collector=/usr/ccs/bin/ld $(LD)
  else
    LD		:= $(PURIFY) $(LD)
  endif
else
ifeq "$(LINK_MODE)" "quantify"
LD		:= $(QUANTIFY) $(LD)
else
ifeq "$(LINK_MODE)" "purecov"
LD		:= $(PURECOV) $(LD)
else
ifeq "$(LINK_MODE)" "purifycov"
LD		:= $(PURIFYCOV) $(LD)
else
error:
	@echo "ERROR!!!! UNKNOWN LINK_MODE($(LINK_MODE))";
	@exit;
endif	# purifycov
endif	# purecov
endif	# quantify
endif	# purify
endif	# normal

ifeq ($(compile64),1) 
  LFLAGS	+= -q64 -bmaxdata:0xFFFFFFFFFFFFFFFF
  ARFLAGS	= -X64 -ruv
  NMFLAGS	= -X32_64 -t x
else
  LFLAGS	+= -q32 -bmaxdata:0x40000000
  ARFLAGS	= -X32 -ruv
  NMFLAGS	= -t x
endif

EFLAGS	= -E -D_POSIX_PTHREAD_SEMANTICS $(CCFLAGS)
SFLAGS	= -S -D_POSIX_PTHREAD_SEMANTICS $(CCFLAGS)
LFLAGS	+= -L.

DLD := $(CXX) # bug in aix.. don't use makeC++Shared... 
DLD = $(CC)
SOLFLAGS = -brtl -bexpall
#SOFLAGS=$(LFLAGS) -qmkshrobj -bM:SRE -bh:5 -bnoentry -bexpall
SOFLAGS=$(LFLAGS) -G -qmkshrobj -bM:SRE -bh:5 -bnoentry -brtl

SKIP_ERR_SYMBOL = -berok

#############################
#  Choose Altibase Build    # 
#############################

# some platform like windows don;t have enough shell buffer for linking. 
# so use indirection file for linking.
NEED_INDIRECTION_BUILD = no

# some platform like aix 4.3 don't need to build shared library 
ifeq "$(OS_MAJORVER)" "4"
NEED_SHARED_LIBRARY=no
else
NEED_SHARED_LIBRARY = yes
endif


# some platform like suse linux/windows have a problem for using libedit.
NEED_BUILD_LIBEDIT = yes

# some platform like a windows don;t have to build jdbc 
NEED_BUILD_JDBC = yes
