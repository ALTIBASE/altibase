#!/bin/sh

all_cnt="`wc -l  recovery.dat | awk ' {print $1 } ' `"
total_cnt=`expr $all_cnt`
fail_cnt=`grep 'Failure' recResult.log | wc -l`
hit_cnt=`grep 'Died' recResult.log | wc -l`
miss_cnt=`grep 'Still' recResult.log | wc -l`

fail_per=`expr $fail_cnt \* 100 \/ $total_cnt`
hit_per=`expr $hit_cnt \* 100 \/ $total_cnt`
miss_per=`expr $miss_cnt \* 100 \/ $total_cnt`

if [ $fail_cnt -eq 0 ] && [ $hit_cnt -ne 0 ] ; then

    echo "Total " $total_cnt  "100" "P"
    echo "Hit " $hit_cnt  $hit_per "P"
    echo "Miss " $miss_cnt $miss_per "P"
    echo "Fail " $fail_cnt $fail_per "P"

else
    echo "Total " $total_cnt  "100" "F"
    echo "Hit " $hit_cnt  $hit_per "F"
    echo "Miss " $miss_cnt $miss_per "F"
    echo "Fail " $fail_cnt $fail_per "F"

fi

