# Makefile with Genernal Options
#
# CVS Info : $Id: x86_solaris_cc.mk 71900 2015-07-24 02:29:00Z gyeongsuk.eom $
#

# 전달되는 외부 변수들

# ID_DIR      : ID 디렉토리
# ID_ACE_ROOT : 라이브러리 패스
# compile64   : 컴파일 환경
# compat5     : CC 5.0 으로?
# compat4     :

# BUILD_MODE의 종류
#   debug		: Debug 모드
#   prerelease      	: -DDEBUG(x) -g (o)
#   release		: release 버젼, 실제 product에 해당
ifndef	BUILD_MODE
	@echo "ERROR BUILD_MODE!!!!"
	@exit
endif

.SUFFIXES: .cpp .$(OBJEXT) .a

CXX	= /opt/SUNWspro/bin/CC
CC	= /opt/SUNWspro/bin/cc
CXXLINT	= /usr/local/parasoft/bin.solaris/codewizard

CXXOPT_DEPENDANCY = -xM1

LD	= $(CXX)

AR	= ar
ARFLAGS	= -ruv

NM 	= /usr/ccs/bin/nm
NMFLAGS	= -t x

PURIFY		= purify -chain-length=100
QUANTIFY	= quantify
PURECOV		= purecov  -max_threads=512 -follow-child-processes
PURIFYCOV   = $(PURIFY) $(PURECOV)

# readline library 설정
ifeq "$(USE_READLINE)" "1"
  READLINE_INCLUDES = -I/usr/local/include/readline
  READLINE_LIBRARY =  -lreadline -ltermcap
endif # use readline library

# LIBS_SHIP added : 20030811
# shared object를 고객이 사용하기 위해 원래의 static 옵션을
# 사용하면, runtime에러가 발생한다. 이를 방지하기 위해
# 클라이언트용 링크 옵션을 별도로 만들어, cli.script에서 사용하도록 함.

ifeq ($(compile64),1) # 64bit
LIBS = -xnolib -Bdynamic -lthread -lposix4 -ldl -lkvm -lkstat -lsocket -lnsl -lgen -lm -lw -lc -Bstatic -liostream -lCrun -ldemangle
LIBS_ODBC = -xnolib -Bdynamic -lthread -lposix4 -ldl -lkvm -lkstat -lsocket -lnsl -lgen -lm -lw -Bstatic
LIBS_SHIP = -lthread -lposix4 -ldl -lkvm -lkstat  -lsocket -lnsl -lgen -ldemangle
else # 32bit release
ifeq ($(compat5),1)
LIBS = -xnolib -Bdynamic -lthread -lposix4 -ldl -lkvm -lkstat -lsocket -lnsl -lgen -lm -lw -lc -Bstatic -liostream -lCrun -ldemangle
LIBS_ODBC = -xnolib -Bdynamic -lthread -lposix4 -ldl -lkvm -lkstat -lsocket -lnsl -lgen -lm -lw -Bstatic
LIBS_SHIP = -lthread -lposix4 -ldl -lkvm -lkstat  -lsocket -lnsl -lgen -ldemangle
else # if compat4
LIBS = -xnolib -Bdynamic -lthread -lposix4 -ldl -lkvm -lkstat  -Bstatic -lsocket -lnsl -lgen -lC -lm -lw -lc -ldemangle
LIBS_ODBC = -xnolib -Bdynamic -lthread -lposix4 -ldl -lkvm -lkstat  -Bstatic -lsocket -lnsl -lgen -lC -lm -lw
LIBS_SHIP = -lthread -lposix4 -ldl -lkvm -lkstat  -lsocket -lnsl -lgen -ldemangle
endif # compat4
endif # compile64

ifeq ($(sb),1)
  SOURCE_BROWSER = -xsb
endif

# 동적 컴파일 옵션 선언
#
# compile mode 의 종류
#	compat4		:
#       compat5		:
#	compile64	:
#
CLASSIC_LIB = -library=iostream,no%Cstd
LIB64_DIRS  = -L/opt/SUNWspro/lib/amd64 -L/usr/lib/amd64
LIB32_DIRS  = -L/opt/SUNWspro/lib -L/usr/lib

ODBC_LIBS = $(LIBS_ODBC)

CLFLAGS =
LFLAGS  =
ifeq "$(BUILD_MODE)" "debug"
else
ifeq "$(BUILD_MODE)" "prerelease"
else
ifeq "$(BUILD_MODE)" "release"
ifeq ($(compile64),1)
  CLFLAGS += -fast
  LFLAGS  += -fast
else
  CLFLAGS += -fast
  LFLAGS  += -fast
endif   # compile64
else
error:
	@echo "ERROR!!!! UNKNOWN BUILD_MODE($(BUILD_MODE))";
	@exit;
endif	# release
endif   # prerelease
endif	# debug

#64bit 컴파일 모드
ifeq ($(compile64),1)
  CLFLAGS += -mt -xarch=amd64 $(LIB64_DIRS)
  LFLAGS  += -mt -xarch=amd64 $(CLASSIC_LIB) $(LIB64_DIRS)
else
#32비트 compat5 모드
ifeq ($(compat5),1)
  CLFLAGS += -mt $(LIB32_DIRS)
  LFLAGS  += -mt $(CLASSIC_LIB) $(LIB32_DIRS)
else
#32비트 compat4 모드
  LFLAGS += -mt -compat=4
endif # 64 bit + compat=5
endif # 64 bit mode


# LINK MODE 종류
#	purify		: purify version
#	quantify	: quantify version
#	purecov		: purecov version
#	purifycov   : purifycov version
############### LINK MODE #####################################3
ifeq "$(LINK_MODE)" "normal"
LD		:=  $(LD)
else
ifeq "$(LINK_MODE)" "purify"
LD		:= $(PURIFY) $(LD)
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



####
#### common rules
####
EFLAGS	= -E $(CCFLAGS)
SFLAGS	= -S $(CCFLAGS)

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

#############################
# TASK-6469 SET MD5
##############################

CHECKSUM_MD5 = md5sum
