cp platform_gcc.mk ~/altidev4/makefiles/platform_gcc.mk
cp platform_x86_g++.GNU ~/altidev4/src/pd/makefiles/platform_x86_g++.GNU
cp platform_macros.GNU ~/altidev4/src/pd/makefiles/platform_macros.GNU
cp configure.in ~/altidev4
autoconf configure.in > ~/altidev4/configure
chmod u+x ~/altidev4/configure
cd ~/altidev4
./configure
