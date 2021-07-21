#
# Makefile for JDBC type 4
#

include ../ulEnv.mk
include $(DEV_DIR)/alticore.mk

# Have to Correct manualy
#debug=1

# Show all message
S_=@

INCLUDES += $(foreach i, $(QP_DIR)/include $(MM_DIR)/include, $(IDROPT)$(i))

# Change this to reflect your setting.  Or override it at the make
# command line:  % make JDK=/home/you/jdk
ifeq "$(JDK)" ""
     JDK=$(JAVA_HOME)
endif

JDBC_DOC       := doc
JDBC_OUTPUT    := bin

JDBC_JAR       := $(ALTI_HOME)/lib/Altibase.jar
JDBC_TRACE_JAR := $(ALTI_HOME)/lib/Altibase_t.jar
DRIVER_DIR     := Altibase/jdbc/driver

VERSION_STR     = $(shell echo `$(ALTI_HOME)/bin/genErrMsg -f` | awk -F"." -v OFS="_" '{print $$1, $$2}')
JDBC_JAR4WAS    = $(ALTI_HOME)/lib/Altibase$(VERSION_STR).jar
DRIVER_DIR4WAS  = Altibase$(VERSION_STR)/jdbc/driver

JUNIT_ZIP      := junit4.10.zip
JUNIT_ZIP_PATH := junit4.10/junit-4.10.jar
JUNIT_ZIP_URL  := http://downloads.sourceforge.net/project/junit/junit/4.10/junit4.10.zip
JUNIT_LIB      := junit-4.10.jar
JUNIT_LIB_DIR  := lib
JUNIT_LIB_PATH := $(JUNIT_LIB_DIR)/$(JUNIT_LIB)
JUNIT_RUNNER   := junit.textui.SimpleTestRunner

#
# Version generator
VERSION_SRC := $(DRIVER_DIR)/AltibaseVersion.java
VERSION_HEADER_SRC:= ${VERSION_SRC}.header
VERSION_TAIL_SRC  := ${VERSION_SRC}.tail

# Trace Flag generator
TRACEFLAG_SRC := $(DRIVER_DIR)/logging/TraceFlag.java

# SQLStateMap generator
SQLSTATEMAP_SRC        := $(DRIVER_DIR)/ex/SQLStateMap.java
SQLSTATEMAP_HEADER_SRC := $(SQLSTATEMAP_SRC).header
SQLSTATEMAP_TAIL_SRC   := $(SQLSTATEMAP_SRC).tail

# JavaVM parametrs by default
JAVA_PARAM= -Xincgc

# JNI
EXT_DIR  := $(UL_DIR_JDBC)/ext
JNI_CLASSES := Altibase.jdbc.driver.JniExt              \
               Altibase.jdbc.driver.JniExtRdmaSocket
INCLUDES += $(IDROPT)$(UL_DIR_JDBC)/include       \
            $(IDROPT)$(JAVA_HOME)/include         \
            $(IDROPT)$(JAVA_HOME)/include/linux

include jdbc_jdk18.mk
include jdbc_shard.mk

ifeq  "$(debug)"  "0"
JAVAC = $(JDK)/bin/javac -g:none -O -nowarn -encoding euc-kr
else
JAVAC = $(JDK)/bin/javac -g -nowarn -encoding euc-kr
endif
JAVAH = $(JDK)/bin/javah


# java
JAVA=$(JDK)/bin/java ${JAVA_PARAM} # -Xincgc

# for remote debug
#PORT=40545
#JAVA=$(JAVA_HOME)/bin/java -client -Xdebug -Xnoagent -Djava.compiler=NONE -Xrunjdwp:transport=dt_socket,server=y,suspend=y,address=$(PORT)

JAR=$(JDK)/bin/jar

.PHONY: all clean compile test-% shobj

ifeq "$(NEED_BUILD_JDBC)"  "yes"
     all:  compile
     sure: compile
else
     all:  jdbc_jni
     sure: jdbc_jni
endif

compile: jdbc jdbc4was jdbc_jni

jdbc: genErrMsg_check jdbc_trace_jar jdbc_jar

genErrMsg_check:
	@VERSION_STR=`$(ALTI_HOME)/bin/genErrMsg -f`;

jdbc_jar: class-clean jdbc_compile copy_service_provider
	$(S_)echo "[JDBC JAR]"
	$(S_)$(JAR) -cmf  $(DRIVER_DIR)/MANIFEST.MF  $(JDBC_JAR) -C $(JDBC_OUTPUT) $(DRIVER_DIR) -C $(JDBC_OUTPUT) META-INF

jdbc_trace_jar:  class-clean jdbc_trace_compile copy_service_provider
	$(S_)echo "[JDBC Trace JAR]"
	$(S_)$(JAR) -cmf  $(DRIVER_DIR)/MANIFEST.MF  $(JDBC_TRACE_JAR) -C $(JDBC_OUTPUT) $(DRIVER_DIR) -C $(JDBC_OUTPUT) META-INF

jdbc_compile: $(VERSION_SRC) $(SQLSTATEMAP_SRC) trace_flag_false
	$(S_)echo "[JDBC compile]"
	$(S_)mkdir -p $(JDBC_OUTPUT)
	$(S_)$(JAVAC) $(JDBC_SRC) $(JDBC_SHARD_SRC) -d $(JDBC_OUTPUT)
	$(S_)$(RM) $(JDBC_OUTPUT)/Altibase/jdbc/driver/logging

jdbc_trace_compile: $(VERSION_SRC) $(SQLSTATEMAP_SRC) trace_flag_true
	$(S_)echo "[JDBC Trace compile]"
	$(S_)mkdir -p $(JDBC_OUTPUT)
	$(S_)$(JAVAC) $(JDBC_SRC) $(JDBC_SHARD_SRC) -d $(JDBC_OUTPUT)

copy_service_provider:
	$(S_)mkdir -p $(JDBC_OUTPUT)/META-INF/services
	$(S_)cp $(DRIVER_DIR)/java.sql.Driver $(JDBC_OUTPUT)/META-INF/services

# A4, A5 support both JDBC.

jdbc4was: jdbc4was_jar

jdbc4was_jar: jdbc4was_compile copy_service_provider
	$(S_)echo "[JDBC2WAS JAR]"
	$(S_)$(JAR) -cmf  $(DRIVER_DIR4WAS)/MANIFEST.MF  $(JDBC_JAR4WAS) -C $(JDBC_OUTPUT) $(DRIVER_DIR4WAS) -C $(JDBC_OUTPUT) META-INF

jdbc4was_compile: $(VERSION_SRC) $(SQLSTATEMAP_SRC) trace_flag_false
	$(S_)echo "[JDBC4WAS compile]"
	$(S_)$(RM) Altibase?_?
	$(S_)cp -r Altibase Altibase$(VERSION_STR)
	$(S_)cd Altibase$(VERSION_STR)/jdbc/driver; find . -name "*.java" -exec perl -pi -e 's/Altibase.jdbc.driver/Altibase$(VERSION_STR).jdbc.driver/g' {} \;
	$(S_)cd Altibase$(VERSION_STR)/jdbc/driver; find . -name "*.MF" -exec perl -pi -e 's/Altibase.jdbc.driver/Altibase$(VERSION_STR).jdbc.driver/g' {} \;
	$(S_)mkdir -p $(JDBC_OUTPUT)
	$(S_)$(JAVAC) $(JDBC_SRC4WAS) $(JDBC_SHARD_SRC4WAS)  -d $(JDBC_OUTPUT)
	$(S_)$(RM) $(JDBC_OUTPUT)/Altibase$(VERSION_STR)/jdbc/driver/logging

# JDBC version generation

$(VERSION_SRC):
	$(S_)echo "[JDBC Version generate]"
	$(S_)cat ${VERSION_HEADER_SRC} > $(VERSION_SRC)
	$(S_)echo "    public static final String ALTIBASE_VERSION_STRING   = \"$(shell $(ALTI_HOME)/bin/genErrMsg -j)\";" >> ${VERSION_SRC};
	$(S_)echo "    private static final int   ALTIBASE_VERSION_VALUES[] = {$(shell echo `$(ALTI_HOME)/bin/genErrMsg -f` | sed 's/\./, /g')};" >> ${VERSION_SRC};
	$(S_)echo "    private static final byte  CM_VERSION_VALUES[]       = {`grep CM_MAJOR_VERSION ${CM_DIR}/include/cmuVersion.h | sed 's/#define [A-Z_]* \([0-9]*\)/\1/'`, `grep CM_MINOR_VERSION ${CM_DIR}/include/cmuVersion.h | sed 's/#define [A-Z_]* \([0-9]*\)/\1/'`, `grep CM_PATCH_VERSION ${CM_DIR}/include/cmuVersion.h | sed 's/#define [A-Z_]* \([0-9]*\)/\1/'`};" >> ${VERSION_SRC};
	$(S_)echo "    private static final String JAVA_VERSION             = \"$(shell java -version 2>&1 | head -1)\";" >> ${VERSION_SRC};
	$(S_)cat ${VERSION_TAIL_SRC} >> $(VERSION_SRC)

version-gen: version-clean $(VERSION_SRC)

version-clean:
	$(S_)$(RM) $(VERSION_SRC)

version-check: $(JDBC_JAR)
	$(S_)java -jar $(JDBC_JAR)

# Trace flag generation

TRACEFLAG = false

trace_flag_true : TRACEFLAG = true
trace_flag_false : TRACEFLAG = false

trace_flag_true trace_flag_false :
	$(S_)$(RM) $(TRACEFLAG_SRC)
	$(S_)echo "[TRACEFLAG generate]"
	$(S_)echo "package Altibase.jdbc.driver.logging;" >> ${TRACEFLAG_SRC};
	$(S_)echo "import Altibase.jdbc.driver.util.AltibaseEnvironmentVariables;" >> ${TRACEFLAG_SRC};
	$(S_)echo "public class TraceFlag " >> ${TRACEFLAG_SRC};
	$(S_)echo "{ " >> ${TRACEFLAG_SRC};
	$(S_)echo "    public static final boolean TRACE_COMPILE = $(TRACEFLAG);" >> ${TRACEFLAG_SRC};
	$(S_)echo "    public static final boolean TRACE_ENABLED = AltibaseEnvironmentVariables.useJdbcTrace();" >> ${TRACEFLAG_SRC};
	$(S_)echo "} " >> ${TRACEFLAG_SRC};

traceflag-clean:
	$(S_)$(RM) $(TRACEFLAG_SRC)

# JDBC SQLStateMap generation

sqlstatemap-gen: sqlstatemap-clean $(SQLSTATEMAP_SRC)

sqlstatemap-bin: SQLStateMapGen$(BINEXT)

$(SQLSTATEMAP_SRC): sqlstatemap-bin
	$(S_)cat ${SQLSTATEMAP_HEADER_SRC} > $(SQLSTATEMAP_SRC)
	$(S_)./SQLStateMapGen >> $(SQLSTATEMAP_SRC)
	$(S_)cat ${SQLSTATEMAP_TAIL_SRC} >> $(SQLSTATEMAP_SRC)

SQLStateMapGen$(BINEXT): $(TARGET_DIR)/src/ul/ulj/SQLStateMapGen.$(OBJEXT) $(TARGET_DIR)/src/ul/uln/ulnErrorDef.$(OBJEXT)
	$(S_)$(LD) $(LFLAGS) $(LDOUT)$@ $^ $(LDROPT)$(ALTICORE_LIBDIR) $(LDROPT)$(ALTI_HOME)/lib $(ALTICORE_LIB) $(LIBS)

sqlstatemap-clean:
	$(S_)$(RM) $(SQLSTATEMAP_SRC) SQLStateMapGen$(BINEXT) $(TARGET_DIR)/src/ul/ulj/SQLStateMapGen.$(OBJEXT)


# JNI modules
jdbc_gen_jni_header:
	$(S_)$(JAVAH) -classpath $(JDBC_JAR) -jni -d ./include $(JNI_CLASSES)

jdbc_jni: jdbc_jni_ext

ifeq "$(ALTI_CFG_OS)" "LINUX"
jdbc_jni_ext: $(EXT_OBJS)
	$(S_)$(RM) $(EXT_SHLIB)
	$(S_)echo "[JDBC JNI MISC compile]"
	$(S_)echo "SO $(EXT_SHLIB)"
	$(S_)$(SOLINK.c) $(SOOUT)$(EXT_SHLIB) $(EXT_OBJS) $(LDROPT)$(ALTICORE_LIBDIR) $(ALTICORE_LIBDIR)/$(LIBPRE)alticore_pic.$(LIBEXT) $(LIBOPT)curses$(LIBAFT) $(LDFLAGS)
else
jdbc_jni_ext:
endif


# JDBC unit test

unittest: unittest_check unittest_prepare unittest_compile unittest_run

unittest_run:
	$(S_)echo "[JDBC unittest - run]"
	$(S_)find test -name '*.java' | xargs grep 'public \+\(static \+\)\?void \+test[a-zA-Z0-9_]*( *)' -l | sed 's~\(test/\|\.java\)~~g' | sed 's~/~.~g' | sed 's~\n~ ~g' | xargs $(JAVA) -Xms256m -Xmx512m -Dfile.encoding=euc-kr -cp $(JDBC_OUTPUT):$(JUNIT_LIB_PATH) $(JUNIT_RUNNER)

unittest_check:
	$(S_)echo "[JDBC unittest - check]"
ifeq ($(wildcard $(JDBC_OUTPUT)/Altibase),)
	$(S_)echo "ERROR: build jdbc first"
	$(S_)exit 1
endif
ifeq ($(wildcard altibase_cli.ini),)
	$(S_)echo "ERROR: cannot find altibase_cli.ini"
	$(S_)exit 1
endif

unittest_prepare:
	$(S_)echo "[JDBC unittest - prepare]"
ifeq ($(wildcard $(JUNIT_LIB_PATH)),)
  ifneq ($(wildcard $(JUNIT_ZIP)),)
	$(S_)$(RM) $(JUNIT_ZIP)
  endif
	$(S_)wget $(JUNIT_ZIP_URL)
	$(S_)unzip -j -o $(JUNIT_ZIP) $(JUNIT_ZIP_PATH)
	$(S_)mkdir -p $(JUNIT_LIB_DIR)
	$(S_)mv $(JUNIT_LIB) $(JUNIT_LIB_DIR)/
	$(S_)$(RM) $(JUNIT_ZIP)
endif
	$(S_)cp -r -f test/resources $(JDBC_OUTPUT)

unittest_compile:
	$(S_)echo "[JDBC unittest - compile]"
	$(S_)$(JAVAC) -encoding euc-kr -cp $(JDBC_OUTPUT):$(JUNIT_LIB_PATH) `find test -name '*.java'` -d $(JDBC_OUTPUT)


class-clean :
	$(S_)echo "[JDBC class clean]"
	$(S_)$(RM) $(JDBC_OUTPUT)

clean: version-clean sqlstatemap-clean traceflag-clean
	$(S_)echo "[JDBC clean]"
	$(S_)$(RM) $(JDBC_OUTPUT)
	$(S_)$(RM) $(JDBC_JAR) ${ALTI_HOME}/$(JDBC_JAR)
	$(S_)$(RM) $(JDBC_TRACE_JAR) ${ALTI_HOME}/$(JDBC_TRACE_JAR)
	$(S_)$(RM) $(ALTI_HOME)/lib/Altibase?_?.jar
	$(S_)$(RM) Altibase?_?
	$(S_)$(RM) $(EXT_OBJ)
	$(S_)$(RM) $(EXT_SHLIB)
	$(S_)$(RM) core* *.log *.map
	
doc:
	$(S_)echo "[JDBC doc]"
	$(S_)rm -rf $(JDBC_DOC)
	$(S_)mkdir $(JDBC_DOC)
	$(S_)javadoc $(JDBC_SRC) $(JDBC_SHARD_SRC) -d $(JDBC_DOC) -package

# Prevent parallel build.
.NOTPARALLEL: compile

