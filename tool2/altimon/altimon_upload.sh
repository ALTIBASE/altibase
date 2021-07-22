#!/bin/ksh

######################################
# function
######################################
send_msg()
{
    send_msg_arg1=$1;
    send_msg_arg2=$2;
    send_msg_arg3=$3;

    telnet $send_msg_arg1 $send_msg_arg2 |&
    while :
    do
        read -p line
        if [ $? -ne 0 ]
        then
            exit -1
        fi
        echo "$line"
        last=`echo $line|cut -c1-6`
        if [ "$last" = "Escape" ]
        then
            print -p $send_msg_arg3;
            print -p "\n";
            echo $send_msg_arg3
            continue
        fi
    done
};

######################################
# configure
######################################
szHostIp="211.188.18.125";
szPort=80;
szFileName="boot.ini";

######################################
# action
######################################
szBoundary="---------------------------7d16d3autonet";

szContent="--${szBoundary}\n"\
"content-disposition: form-data; name=\"file2\"; \n"\
"\n"\
"abcdef\n"\
"--${szBoundary}\n"\
"content-disposition: form-data; name=\"file1\"; filename=\"${szFileName}\"\n"\
"Content-Type: text/plain\n"\
"\n"\
"abcdef"\
"\n"\
"--${szBoundary}--\n"\
"--${szBoundary}--\n"\
"\n";

szContentLength=`echo ${szContent} | wc -c`;
let szContentLength=szContentLength-1

szHttpReq="POST /t_app/t_upload_action.jsp HTTP/1.0\n"\
"Accept: image/gif, image/x-xbitmap, image/jpeg, image/pjpeg, application/vnd.ms-powerpoint, application/vnd.ms-excel, application/m
sword, */*\n"\
"Accept-Language: ko\n"\
"Content-Type: multipart/form-data; boundary=${szBoundary}\n"\
"Accept-Encoding: gzip, deflate \n"\
"User-Agent: f_upload\n"\
"Host: ${szHostIp}\n"\
"Content-Length: ${szContentLength}\n"\
"Connection: Keep-Alive\n"\
"Cache-Control: no-cache \n"\
"\n"\
${szContent};

echo $szHttpReq;

send_msg $szHostIp $szPort "${szHttpReq}"

