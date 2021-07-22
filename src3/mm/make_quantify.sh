#!/usr/bin/sh -v
cd src/mmi
make mmiMgr_quantify
cd ..
make lib
cd main
make altibase_quantify
cp altibase $ALTIBASE_HOME/bin
cd ../..

