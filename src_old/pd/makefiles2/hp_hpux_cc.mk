# Makefile with Genernal Options
#
# CVS Info : $Id: hp_hpux_cc.mk 71900 2015-07-24 02:29:00Z gyeongsuk.eom $
#

# 전달되는 외부 변수들

# ID_DIR      : ID 디렉토리 
# compile64   : 컴파일 환경
# compat5     : CC 5.0 으로?
# compat4     : 

ifndef	BUILD_MODE
	echo "not defined BUILD_MODE!!"
	exit;
endif

.SUFFIXES: .cpp .$(OBJEXT) .a

CXX	= aCC
CC	= cc

LD	= $(CXX)

AR	= ar
ARFLAGS	= -ruv

NM 	= /usr/bin/nm
NMFLAGS	= -t x

PURIFY		= purify -chain-length=100
QUANTIFY	= quantify
PURECOV		= purecov
PURIFYCOV   = $(PURIFY) $(PURECOV)



PURIFY = purify -chain-length=100

#ifeq ($(compile64),1)
#PURIFY = purify64 -chain-length=100
#else
#PURIFY = purify32 -chain-length=100
#endif

# Library : static library 처리 

# readline library 설정

ifeq "$(USE_READLINE)" "1"
READLINE_INCLUDES = -I/usr/local/include/readline
READLINE_LIBRARY =  -lreadline -ltermcap 
#error:
#	echo "$(READLINE_LIBRARY)";
endif # use readline library 


ifeq "$(BUILD_MODE)" "release"
LIBS     =   $(READLINE_LIBRARY) -lxti -lpthread -lrt -ldld
else
LIBS     =   $(READLINE_LIBRARY) -lxti -lpthread -lrt -ldld
endif

ifeq ($(sb),1)
SOURCE_BROWSER = -xsb
endif

AOPT        = -xar
#DEFINES		= -D_REENTRANT

CXXOPT_DEPENDANCY = -xM1

# 동적 컴파일 옵션 선언
# // for solving include problem... by gamestar

#
# compile mode 의 종류
#	compat4		:
#       compat5		:
#	compile64	:
#
# 매크로 선언
CLASSIC_LIB = 
LIB64_DIRS  = 
LIB32_DIRS  = 
LFLAGS	= -L.
#64bit 컴파일 모드 
ifeq ($(compile64),1)
ifeq ($(OS_MINORVER),22)
LFLAGS += -AP +DD64 -Wl,+vnocompatwarnings $(CLASSIC_LIB) $(LIB64_DIRS)
else
ifeq ($(OS_MINORVER),23)
LFLAGS += -AP +DD64 -Wl,+vnocompatwarnings $(CLASSIC_LIB) $(LIB64_DIRS)
else
ifeq ($(OS_MINORVER),31)
LFLAGS += -AP +DD64 -Wl,+vnocompatwarnings $(CLASSIC_LIB) $(LIB64_DIRS)
else
LFLAGS += +DA2.0W +DS2.0 -Wl,+vnocompatwarnings $(CLASSIC_LIB) $(LIB64_DIRS)
endif
endif
endif
else 
#32비트  모드
ifeq ($(OS_TARGET),IA64_HP_HPUX)
LFLAGS += -AP +DA1.1 +DS1.1  -N -Wl,+s
else
LFLAGS += +DA1.1 +DS1.1  -N -Wl,+s
endif
endif # 64 bit mode

ifeq "$(BUILD_MODE)" "debug"
LFLAGS	+= -g  -Wl,+s -z  -DDEBUG
else
ifeq "$(BUILD_MODE)" "prerelease"
LFLAGS	+= -g
else
ifeq "$(BUILD_MODE)" "release"
LFLAGS	+= 
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

EFLAGS	= -E $(CCFLAGS)
SFLAGS	= -S $(CCFLAGS)
ifeq ($(compile64),1)
SOLFLAGS = -Wl,+forceload,-lqp,+noforceload
else
SOLFLAGS = -Wl,-E,-Fl,$(ALTI_HOME)/lib/libqp.a
endif

ODBC_LIBS =
LIBS_SHIP = $(LIBS)

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

CHECKSUM_MD5 = openssl dgst -md5
