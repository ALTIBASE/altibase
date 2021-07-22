#!/usr/local/bin/bash

cat $ALTIBASE_HOME/conf/recovery.dat | sort  -d > $ALTIBASE_HOME/conf/rec_sort.tmp;
cat $ALTIBASE_HOME/conf/hitpoint.dat | sort  -d > $ALTIBASE_HOME/conf/hit_sort.tmp;

diff $ALTIBASE_HOME/conf/rec_sort.tmp $ALTIBASE_HOME/conf/hit_sort.tmp | grep '<' | awk -F' ' '{ print $2" "$3" "$4; }' > $ALTIBASE_HOME/conf/misspoint.dat

#tkdiff $ALTIBASE_HOME/conf/misspoint.dat $ALTIBASE_HOME/conf/hit_sort.tmp 

rm -f $ALTIBASE_HOME/conf/*_sort.tmp
