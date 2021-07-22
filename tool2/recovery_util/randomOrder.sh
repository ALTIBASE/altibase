#!/bin/sh

fileName=$1  #재정렬할 파일명 $1인자로 받음.

count=0				#count 초기화
fileSize=`cat $fileName | wc -l`	#파일사이즈
echo "fileSize : " $fileSize
loop=`expr $fileSize \* 3`			#mix 횟수

today=`date +"%d"`
second=`date +"%S"`
lineNum=`expr $today + $second` #변경할 라인 초기화

while [ $count -lt $loop ]
do
	cp $fileName tempFile
	lineNum=`expr $count \* 3 + $lineNum`	#바꾸려는 라인수 계산
	oddCheck=`expr $lineNum \% $fileSize \% 2`
	lineNum=`expr $lineNum \% $fileSize`
	
	if [ $lineNum -lt 2 ]
	then
		    lineNum=`expr $lineNum + 2`
	fi
		
	head -$lineNum $fileName | tail -1 >> $fileName
    sed ''$lineNum'd' $fileName > tempFile
	mv tempFile $fileName
	count=`expr $count + 1`
done

#path=`echo ${ATAF_TEST_CASE//\//\\\/}`
#sed 's/\.\./'$path'/g' $fileName > tempFile
#mv tempFile $fileName
