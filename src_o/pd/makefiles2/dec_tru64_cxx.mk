# Makefile with Genernal Options
#
# CVS Info : $Id: dec_tru64_cxx.mk 26440 2008-06-10 04:02:48Z jdlee $
#

# 전달되는 외부 변수들

# ID_DIR      : ID 디렉토리 
# ID_ACE_ROOT : 라이브러리 패스
# compile64   : 컴파일 환경
# compat5     : CC 5.0 으로?
# compat4     : 

ifndef	BUILD_MODE
	echo "not defined BUILD_MODE!!"
	exit;
endif

.SUFFIXES: .cpp .$(OBJEXT) .a

CXX	= /usr/bin/cxx
CC	= /usr/bin/cc

LD	= $(CXX)

AR	= ar
#ARFLAGS	= -ruv

NM 	= /usr/bin/nm
NMFLAGS	= -t x

QUANTIFY	= quantify
PURECOV		= purecov
PURIFYCOV   = $(PURIFY) $(PURECOV)
PURIFY = purify64 -chain-length=100

# Library : static library 처리 

# readline library 설정

ifeq "$(USE_READLINE)" "1"
READLINE_INCLUDES = -I/usr/local/include/readline
READLINE_LIBRARY =  -lreadline -ltermcap 
#error:
#	echo "$(READLINE_LIBRARY)";
endif # use readline library 


LD_CC = -lcxx -laio

ifeq "$(BUILD_MODE)" "release"
#LIBS     =   $(READLINE_LIBRARY) -lxti -lpthread -lrt -ldld
LIBS     =   $(READLINE_LIBRARY) -ltli -lrt -lpthread -lm $(LD_CC)
else
#LIBS     =   $(READLINE_LIBRARY) -lxti -lpthread -lrt -ldld
LIBS     =   $(READLINE_LIBRARY) -ltli -lrt -lpthread -lm $(LD_CC)
endif
LIBS_SHIP	= $(LIBS)

# 매크로 선언
CLASSIC_LIB = 
LIB64_DIRS  = 
LIB32_DIRS  = 

# compile mode 의 종류
#	compat4		:
#       compat5		:
#	compile64	:
#
#64bit 컴파일 모드 
ifeq ($(compile64),1)

#EXTRA_CXXOPT += +DA2.0 +DS2.0 $(CLASSIC_LIB) $(TEMPLATE) $(COMPAT64_ACE_FLAG) 
#EXTRA_LOPT += -Wl,+vnocompatwarnings $(CLASSIC_LIB) $(LIB64_DIRS)
EXTRA_CXXOPT += $(CLASSIC_LIB) $(TEMPLATE) $(COMPAT64_ACE_FLAG) 
EXTRA_LOPT += $(CLASSIC_LIB) $(LIB64_DIRS)

else 
#32비트  모드
#EXTRA_CXXOPT += +DA1.1 +DS1.1 $(TEMPLATE) $(COMPAT4_ACE_FLAG)
#EXTRA_LOPT += +DA1.1 +DS1.1
EXTRA_CXXOPT += $(TEMPLATE) $(ACE_FLAG)
EXTRA_LOPT += 

endif # 64 bit mode

#CXXOPT		= -c +w -mt -D_POSIX_PTHREAD_SEMANTICS -noex
#EOPT		= -E -mt -D_POSIX_PTHREAD_SEMANTICS -noex
#SOPT		= -S -mt -D_POSIX_PTHREAD_SEMANTICS -noex
CXXOPT		= -c 
EOPT		= -E
SOPT		= -S
LOPT		= -L. 
AOPT        = -xar
#DEFINES		= -D_REENTRANT


ifeq "$(OS_MAJORVER)" "4"
    DEC_VERSION=0x40F
else
    ifeq "$(OS_MAJORVER)" "5"
        DEC_VERSION=0x500
    endif
endif

#CXXOPT_DEPENDANCY = -xM1


# BUILD_MODE의 종류
#	debug		: Debug 모드
#   prerelease      : -DDEBUG(x) -g (o)
#	release		: release 버젼, 실제 product에 해당

# LINK MODE 종류 
#	purify		: purify version
#	quantify	: quantify version
#	purecov		: purecov version
#	purifycov   : purifycov version

############### BUILD MODE #####################################3

ifeq "$(BUILD_MODE)" "debug"
ACE_FLAG = -pthread -DDIGITAL_UNIX=${DEC_VERSION} -g -O0 -w -msg_disable 9  -DACE_NO_INLINE -DACE_LACKS_ACE_TOKEN -DACE_LACKS_ACE_OTHER
EXTRA_CXXOPT	+= $(ACE_FLAG) -g $(SOURCE_BROWSER) -DDEBUG
EXTRA_LOPT	+= -g

else
ifeq "$(BUILD_MODE)" "prerelease"
ACE_FLAG = -pthread -DDIGITAL_UNIX=${DEC_VERSION} -g -O0 -w -msg_disable 9 -D__ACE_INLINE__ -DACE_LACKS_ACE_TOKEN -DACE_LACKS_ACE_OTHER
EXTRA_CXXOPT	+= $(ACE_FLAG) -g
EXTRA_LOPT	+= -g

else
ifeq "$(BUILD_MODE)" "release"
ACE_FLAG = -pthread -DDIGITAL_UNIX=${DEC_VERSION} -DACE_NDEBUG -w -msg_disable 9 -D__ACE_INLINE__ -DACE_LACKS_ACE_TOKEN -DACE_LACKS_ACE_OTHER
EXTRA_CXXOPT	+= $(ACE_FLAG) -fast -O0 -tune ev6 -arch ev6
EXTRA_LOPT	+= -pthread -fast -O0 -tune ev6 -arch ev6
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

CXXFLAGS	= $(CXXOPT) $(EXTRA_CXXOPT)

EFLAGS	= $(EOPT) $(EXTRA_CXXOPT)
SFLAGS	= $(SOPT) $(EXTRA_CXXOPT)
LFLAGS	= $(LOPT) $(EXTRA_LOPT)


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
