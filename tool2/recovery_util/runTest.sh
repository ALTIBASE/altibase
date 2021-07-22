#!/usr/local/bin/bash 

server kill

. ./art.env

rm -f  recResult.log
rm -f $ALTIBASE_HOME/trc/killPoint.dat
rm -f $ALTIBASE_HOME/conf/recovery.dat
rm -f $ALTIBASE_HOME/conf/hitpoint.dat

destroydb -n mydb
createdb -M 1

sh gen.sh
gawk -v LOG="recResult.log" -v LIST="$ALTIBASE_HOME/conf/recovery.dat" -f doRecTest.awk

