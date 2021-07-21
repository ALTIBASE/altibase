include ../dkEnv.mk

ANT_HOME := $(shell pwd)/tools/ant
PATH := ${PATH}:${ANT_HOME}/bin

S_=@
ANT="${ANT_HOME}/bin/ant"
AFLAGS="--noconfig"

VERSION_SRC := src/com/altibase/altilinker/Version.java
VERSION_HEADER_SRC:= ${VERSION_SRC}.header
VERSION_TAIL_SRC  := ${VERSION_SRC}.tail 

all: compile version-check
sure: compile version-check

version-gen:
	$(S_)echo "[Altilinker Version generate]"
	$(S_)cat ${VERSION_HEADER_SRC} > $(VERSION_SRC)
	$(S_)echo "    public static final String ALTIBASE_VERSION_STRING   = \"$(shell $(ALTI_HOME)/bin/genErrMsg -j)\";" >> ${VERSION_SRC};
	$(S_)echo "    private static final int   ALTIBASE_VERSION_VALUES[] = {$(shell echo `$(ALTI_HOME)/bin/genErrMsg -f` | sed 's/\./, /g')};" >> ${VERSION_SRC};
	$(S_)echo "    private static final String JAVA_VERSION             = \"$(shell java -version 2>&1 | head -1)\";" >> ${VERSION_SRC};
	$(S_)cat ${VERSION_TAIL_SRC} >> $(VERSION_SRC)
	$(S_)echo "Version Src : $(VERSION_SRC)"

version-clean:
	$(S_)echo "[Delete Version File]"
	$(S_)$(RM) $(VERSION_SRC)

version-check: compile
	$(S_)echo "[Altilinker Version]"
	$(S_)java -jar dist/altilinker.jar -v

compile: version-gen
ifeq "$(BUILD_MODE)" "debug"
	$(S_)$(ANT) $(AFLAGS) package_debug
else
	$(S_)$(ANT) $(AFLAGS) package_release
endif
	$(S_)$(COPY) dist/altilinker.jar ${ALTI_HOME}/bin

clean: version-clean
	$(S_)$(ANT) $(AFLAGS) clean

