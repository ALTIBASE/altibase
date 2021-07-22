#!/usr/bin/sh -v
cd src/mmi
make mmiMgr_purecov
cd ..
make lib
cd main
make altibase_purecov
cp altibase $ALTIBASE_HOME/bin
cd ../..

