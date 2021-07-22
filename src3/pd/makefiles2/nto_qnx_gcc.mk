# Makefile with Genernal Options
#
# CVS Info : $Id: nto_qnx_gcc.mk 26440 2008-06-10 04:02:48Z jdlee $
#

# 전달되는 외부 변수들
# ID_DIR      : SM 디렉토리 
# ID_ACE_ROOT : 라이브러리 패스
# compile64   : 컴파일 환경

ifndef	BUILD_MODE
	@echo "ERROR BUILD_MODE!!!!"
	@exit
endif

.SUFFIXES: .cpp .$(OBJEXT) .a

CXXOPT_DEPENDANCY = -MM
EFLAGS	= -E $(CCFLAGS) 
SFLAGS	= -S $(CCFLAGS) 
AOPT    =

NM 	    = /usr/bin/nm
NMFLAGS	= -t x

# readline library 설정
ifeq "$(USE_READLINE)" "1"
READLINE_INCLUDES = -I/usr/local/include/readline
READLINE_LIBRARY =  -lreadline -ltermcap 
endif # use readline library 

# LIBS 중복
LIBS     = $(READLINE_LIBRARY) -lsocket

# LDFLAGS와 중복
LFLAGS  =
# BUILD_MODE의 종류
# debug	      : Debug 모드
# prerelease  : -DDEBUG(x) -g (o)
# release     : release 버젼, 실제 product에 해당
ifeq "$(BUILD_MODE)" "debug"
    LFLAGS = -g
else
ifeq "$(BUILD_MODE)" "prerelease"
    LFLAGS = -g
else
ifeq "$(BUILD_MODE)" "release"
    LFLAGS = -O3 -funroll-loops
else
    error:
    @echo "ERROR!!!! UNKNOWN BUILD_MODE($(BUILD_MODE))";
    @exit;
endif # release
endif # prerelease
endif # debug

############### LINK MODE #####################################3
PURIFY	  = purify -chain-length=100
QUANTIFY  = quantify
PURECOV	  = purecov
PURIFYCOV = $(PURIFY) $(PURECOV)

ifeq "$(LINK_MODE)" "normal"
  LD := $(LD)
else
ifeq "$(LINK_MODE)" "purify"
  LD := $(PURIFY) $(LD)
else
ifeq "$(LINK_MODE)" "quantify"
  LD := $(QUANTIFY) $(LD)
else
ifeq "$(LINK_MODE)" "purecov"
  LD := $(PURECOV) $(LD)
else
ifeq "$(LINK_MODE)" "purifycov"
  LD := $(PURIFYCOV) $(LD)
else
error:
  @echo "ERROR!!!! UNKNOWN LINK_MODE($(LINK_MODE))";
  @exit;
endif  # purifycov
endif  # purecov
endif  # quantify
endif  # purify
endif  # normal


#############################
#  Choose Altibase Build    # 
#############################

# some platform like windows don;t have enough shell buffer for linking. 
# so use indirection file for linking.
NEED_INDIRECTION_BUILD = no

# some platform like aix 4.3 don't need to build shared library 
NEED_SHARED_LIBRARY = yes

# some platform like suse linux/windows have a problem for using libedit.
NEED_BUILD_LIBEDIT = yes

# some platform like a windows don;t have to build jdbc 
NEED_BUILD_JDBC = yes
