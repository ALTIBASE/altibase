%ALTIBASEDEV%\win32-build\bin\flex  -Cfar  -omtcddl.cpp mtcddl.l
%ALTIBASEDEV%\win32-build\bin\sed s,"extern \"C\" int isatty YY_PROTO(( int ));","", < mtcddl.cpp > mtcddl_tmp
echo Y | move mtcddl_tmp mtcddl.cpp

