REM Configuration specific to HDB or HDB DA Edition
@if "%1"=="hdb" (
    cat %ALTIBASE_DEV%\src\pd\port\windows\vars.mk.hdb > %ALTIBASE_DEV%\vars.mk
	cat %ALTIBASE_DEV%\src\pd\port\windows\config-files\idConfig.h.hdb > %ALTIBASE_DEV%\src\id\idConfig.h
) else if "%1"=="xdbcli" (
    cat %ALTIBASE_DEV%\src\pd\port\windows\vars.mk.xdb > %ALTIBASE_DEV%\vars.mk
	cat %ALTIBASE_DEV%\src\pd\port\windows\config-files\idConfig.h.xdb > %ALTIBASE_DEV%\src\id\idConfig.h
) else (
    echo Error: missing mandatory option. >&2
    echo Usage: configure64.bat {hdb/xdbcli} [release] [--enable-unittest/--enable-limitcheck/--enable-ssl] >&2
    exit /B 1
)

REM Copy files
cat %ALTIBASE_DEV%\src\pd\port\windows\config-files\idConfig.h.Win32 >> %ALTIBASE_DEV%\src\id\idConfig.h
@REM bug-25127 windows server 2008 porting. append OS_MAJORVER to idConfig.h
call %ALTIBASE_DEV%\src\pd\port\windows\ntversion.bat
cp %ALTIBASE_DEV%\src\pd\port\windows\config-files\config-linkage.h   %ALTIBASE_DEV%\src\pd\makeinclude

cp %ALTIBASE_DEV%\src\core\include\acpConfigPlatform.h.win32 %ALTIBASE_DEV%\src\core\include\acpConfigPlatform.h
cp %ALTIBASE_DEV%\makefiles\config.mk.win32 %ALTIBASE_DEV%\makefiles\config.mk
cp %ALTIBASE_DEV%\makefiles\platform_msvc.mk %ALTIBASE_DEV%\makefiles\platform.mk

REM call %ALTIBASE_DEV%\src\pd\port\windows\copy.bat

cl 2>%ALTIBASE_DEV%\MSC_VERSION

@del env.mk
REM Create "env.mk".
=========================
@if "%2"=="release" goto UseRelease

@echo "############ [DEBUG] Mode Configurated ###########"

cat %ALTIBASE_DEV%\src\pd\port\windows\header_debug.txt >>%ALTIBASE_DEV%\vars.mk
cat %ALTIBASE_DEV%\src\pd\port\windows\version_check.txt >%ALTIBASE_DEV%\env.mk

@if "%2"=="--enable-unittest" (
    @if "%ALTICORE_HOME%"=="" (
        echo DO_UNITTEST=no>>%ALTIBASE_DEV%\vars.mk
    ) else (
        echo DO_UNITTEST=yes>>%ALTIBASE_DEV%\vars.mk
        echo ALTICORE_HOME=%ALTICORE_HOME%>>%ALTIBASE_DEV%\vars.mk
        echo UNITTEST_USE_STATIC_LIB=no>>%ALTIBASE_DEV%\vars.mk
    )
) else (
    echo DO_UNITTEST=no>>%ALTIBASE_DEV%\vars.mk
)

@if "%2"=="--enable-limitcheck" (
        echo ALTIBASE_LIMIT_CHECK=yes>>%ALTIBASE_DEV%\env.mk
) else (
    echo ALTIBASE_LIMIT_CHECK=no>>%ALTIBASE_DEV%\env.mk
)

cat %ALTIBASE_DEV%\src\pd\port\windows\env.mk.body >>%ALTIBASE_DEV%\env.mk
echo ALTI_CFG_BITTYPE=32>>%ALTIBASE_DEV%\vars.mk

@goto end

:UseRelease

@echo "############ [RELEASE] Mode Configurated ###########"
cat %ALTIBASE_DEV%\src\pd\port\windows\header_release.txt >>%ALTIBASE_DEV%\vars.mk
cat %ALTIBASE_DEV%\src\pd\port\windows\version_check.txt >%ALTIBASE_DEV%\env.mk

@if "%3"=="--enable-unittest" (
    @if "%ALTICORE_HOME%"=="" (
        echo DO_UNITTEST=no>>%ALTIBASE_DEV%\vars.mk
    ) else (
        echo DO_UNITTEST=yes>>%ALTIBASE_DEV%\vars.mk
        echo ALTICORE_HOME=%ALTICORE_HOME%>>%ALTIBASE_DEV%\vars.mk
        echo UNITTEST_USE_STATIC_LIB=no>>%ALTIBASE_DEV%\vars.mk
    )
) else (
    echo DO_UNITTEST=no>>%ALTIBASE_DEV%\vars.mk
)

cat %ALTIBASE_DEV%\src\pd\port\windows\env.mk.body >>%ALTIBASE_DEV%\env.mk
echo ALTI_CFG_BITTYPE=32>>%ALTIBASE_DEV%\vars.mk

:end
=========================

