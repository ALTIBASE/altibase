@echo off
rem ***********************************************************************
rem ** bug-25127 windows server 2008 porting
rem ** NT majoer 버전 번호가 기존에는 5로 고정되어 있었는데 
rem ** 2008부터 NT 6이므로 ver.exe 프로그램으로 NT 버전을 구하여 
rem ** 패키지명으로 사용하도록 한다.
rem ** ver.exe 실행시 결과: Microsoft Windows [Version 6.0.6002]
rem ** Version 6를 찾으면 idConfig.h 파일뒤에 OS_MAJORVER을 6으로 추가
rem ** 못찾으면 예전대로 5로 추가한다.
rem ** 본 bat 파일은 configure64.bat (or configure.bat)에서 호출한다.
rem ***********************************************************************

ver | findstr /c:"Version 6" > NUL
if %errorlevel% equ 0 goto WINNT6

echo #define OS_MAJORVER 5 >> %ALTIBASE_DEV%\src\id\idConfig.h
goto EOF

:WINNT6
echo #define OS_MAJORVER 6 >> %ALTIBASE_DEV%\src\id\idConfig.h

:EOF
@echo on
