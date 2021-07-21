
cd %ALTIBASE_DEV%
copy ut\ses3\src\include\ses.h                 %ALTIBASE_HOME%\include
copy ut\ses3\src\include\sesMultiThread.h      %ALTIBASE_HOME%\include
copy ut\ses3\src\include\msesADLDC.h           %ALTIBASE_HOME%\include
copy ut\utm\src\aexport.properties.org         %ALTIBASE_HOME%\conf
copy ul\src\include\windows-odbc\uloWindowsOdbc.h %ALTIBASE_HOME%\include


cd install
make
mkdir %ALTIBASE_HOME%\install
cp altibase_env.mk %ALTIBASE_HOME%\install\altibase_env.mk
