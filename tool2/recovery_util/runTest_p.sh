#!/usr/local/bin/bash

echo $ALTIBASE_HOME

rm -f *.out *.log
rm -f $ALTIBASE_HOME/trc/killPoint.dat
rm -f $ALTIBASE_HOME/conf/recovery*.dat
rm -f $ALTIBASE_HOME/conf/hitpoint.dat

date;

make clean2;
make;

sh gen.sh;
./split 200 3 $ALTIBASE_HOME/conf/recovery

artp $ALTIBASE_HOME/conf/recovery_1.dat 100 1 &> 1.out &
artp $ALTIBASE_HOME/conf/recovery_2.dat 200 2 &> 2.out &
artp $ALTIBASE_HOME/conf/recovery_3.dat 300 3 &> 3.out &

wait;
date;

cat recResult_1.log >> recResult.log
cat recResult_2.log >> recResult.log
cat recResult_3.log >> recResult.log

