%ALTIBASEDEV%\win32-build\bin\flex  -Cfar  -omtddl.cpp mtddl.l
%ALTIBASEDEV%\win32-build\bin\sed s,"extern \"C\" int isatty YY_PROTO(( int ));","", < mtddl.cpp > mtddl_tmp
echo Y | move mtddl_tmp mtddl.cpp

