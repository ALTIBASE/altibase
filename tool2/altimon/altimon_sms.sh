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
szPhone="01195687502";
szSendWin="$1";
szHostIp="150.204.1.60";
szPort=7011;

######################################
# action
######################################
szContentLength=0
szContent="siteInfo=nat&userNum=natebuiling&smsTransphone=${szPhone}&SendWin=${szSendWin}&Callphone=${szPhone}";
szContentLength=`echo "$szContent" | wc -c | sed 's/ //g'`;

szHttpReq="POST /jsp/sms2/SMSCgi.jsp HTTP/1.0\n"\
"Accept: image/gif, image/x-xbitmap, image/jpeg, image/pjpeg, */*\n"\
"Accept-Language: ko\n"\
"Content-Type: application/x-www-form-urlencoded\n"\
"Accept-Encoding: zip, deflate\n"\
"User-Agent: Mozilla/4.0 (compatible; MSIE 5.01; Windows NT 5.0)\n"\
"Host: ${szHostIp}\n"\
"Content-Length: ${szContentLength}\n"\
"Connection: Close\n"\
"Cache-Control: no-cache \n"\
"\n"\
"${szContent}";

send_msg $szHostIp $szPort "${szHttpReq}"

