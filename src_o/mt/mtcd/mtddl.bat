%ALTIBASEDEV%\win32-build\bin\flex  -Cfar  -omtddl.cpp mtddl.l
%ALTIBASEDEV%\win32-build\bin\sed s,"class istream;","#include <iostream>\n\r using std::istream;\n\r using std::ostream;\n\r using std::cin;\n\r using std::cout;\n\r using std::cerr;\n\r", < mtddl.cpp > mtddl_tmp
echo Y | move mtddl_tmp mtddl.cpp

