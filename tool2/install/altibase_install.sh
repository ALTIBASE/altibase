#!/bin/sh

##############################
# Macro Define
##############################

TODAY=`date "+%y%m%d_%H%M%S"`

SYSTEM=`uname`
WHOAMI=`whoami`
PWD=`pwd`
INIT_DIR=`echo $PWD`
HOME=`echo $HOME`
SH=`echo $SHELL | awk '{print "basename ", $1}' | sh`
KMTUNE=/usr/sbin/kmtune

if [ $SYSTEM = "HP-UX" ]
then
	type=`uname -m`
	if [ $type = "ia64" ]
	then
		SYSTEM="HP-RX"
	fi

    GZIP=/usr/contrib/bin/gzip
else
    GZIP=gzip
fi

ALTIBASE_HOME="${HOME}/altibase_home"
ADMIN="${ALTIBASE_HOME}/bin/isql -u sys -p manager -sysdba"
ISQL="${ALTIBASE_HOME}/bin/isql -u sys -p manager"
CUR_SYSTEM_ENV="${ALTIBASE_HOME}/conf/altibase.current_system_env.$TODAY"
REC_SYSTEM_ENV="${ALTIBASE_HOME}/conf/altibase.recommend_system_env"
INSTALL_RESULT="${INIT_DIR}/altibase_install.log"
ALTI_PROFILE="${ALTIBASE_HOME}/conf/altibase.profile"
ORG_PROFILE="${ALTIBASE_HOME}/conf/profile.org"
TMP="${INIT_DIR}/altibase_tmp.txt"

ALTI_PKG="${INIT_DIR}/.alti_pkg_list.txt"
INSTALL_STEP="${INIT_DIR}/.alti_install_step.txt"

DB_NAME="mydb"
MEM_DB_DIR="?/dbs"
OPEN_FILES=2048

echo > $INSTALL_RESULT


##############################
# Function Define
##############################

get_string ()
{
#	if [ $SYSTEM = "HP-UX" -o $SYSTEM = "HP-RX" ]
#	if [ $SH = "ksh" -o $SH = "bash" ]
	if [ $SH = "ksh" ]
	then
		echo "$STRING"\\c
	else
		echo -n "$STRING"
	fi
}

chk_error ()
{
	if [ $? != 0 ]
	then
		cat $TMP >> $INSTALL_RESULT
		echo >> $INSTALL_RESULT
		result=-1
	else
		result=0
	fi
}

chk_os ()
{
    if [ $SYSTEM != "HP-UX" -a $SYSTEM != "AIX" -a $SYSTEM != "SunOS" -a $SYSTEM != "Linux" -a $SYSTEM != "OSF1" -a $SYSTEM != "HP-RX" ]
    then
        echo "  [Error] Sorry, not support altibase_install.sh for this platform!!!"
        echo "          You can install altibase manually."
        exit
    fi
}

install_pkg ()
{
    #echo -n "  Would you like to install altibase into $ALTIBASE_HOME? (yes) "
    STRING="  Would you like to install altibase into $ALTIBASE_HOME? (yes) "
	get_string
    read res
    if [ x$res != "x" ]
    then
        if [ $res != "yes" -a $res != "y" -a $res != "YES" -a $res != "Y" ]
        then
            #echo -n "  Please input the directory for install altibase : "
            STRING="  Please input the directory for install altibase : "
			get_string
            read res
            if [ x$res = "x" ]
            then
                echo "  [Error] You don't specify the directory for install altibase."
                echo "          Please try perform altibase_install.sh again."
                exit
            else
                ALTIBASE_HOME=$res
            fi
        fi
    fi

	ADMIN="${ALTIBASE_HOME}/bin/isql -u sys -p manager -sysdba"
	ISQL="${ALTIBASE_HOME}/bin/isql -u sys -p manager"
	CUR_SYSTEM_ENV="${ALTIBASE_HOME}/conf/altibase.current_system_env.$TODAY"
	REC_SYSTEM_ENV="${ALTIBASE_HOME}/conf/altibase.recommend_system_env"
	ALTI_PROFILE="${ALTIBASE_HOME}/conf/altibase.profile"
	ORG_PROFILE="${ALTIBASE_HOME}/conf/profile.org"

    if [ -d $ALTIBASE_HOME ]
    then
		echo
        echo "  $ALTIBASE_HOME already exists."
		echo
        echo "  Of the following two options : "
        echo
        echo "   1) Backup $ALTIBASE_HOME"
        echo "   *) Overwrite $ALTIBASE_HOME"
        echo
        #echo -n "  Which would you like to perform? 1) "
        STRING="  Which would you like to perform? 1) "
		get_string
        read res
        if [ x$res = "x" ] || [ $res = "1" ]
        then
			mv $ALTIBASE_HOME ${ALTIBASE_HOME}_${TODAY}
        	mkdir $ALTIBASE_HOME
		fi
	else
        mkdir $ALTIBASE_HOME
    fi

    ls -1 altibase-*gz | grep -v client > $ALTI_PKG
    pkg_cnt=`wc -l $ALTI_PKG | awk '{print $1}'`
    if [ $pkg_cnt -eq 0 ]
    then
        echo "  No altibase package in current directory. "
        echo
        #echo -n "  Please input the altibase package name (fullpath) : "
        STRING="  Please input the altibase package name (fullpath) : "
		get_string
        read res
        if [ x$res = "x" ]
        then
			echo
            echo "  [Error] You don't specify the altibase package name. "
            exit;
        else
            PKG_NAME=$res
        fi
    else
		echo
        echo "  You have the altibase packages of the following : "
        echo
	
		awk '{ print "   " NR ") " $0 }' $ALTI_PKG
		cnt=`wc -l $ALTI_PKG | awk '{print $1+1}'`
        echo "   $cnt) input the other altibase package yourself"
   
        echo
        #echo -n "  Please choose one : 1) "
        STRING="  Please choose one : 1) "
		get_string
        read res
        if [ x$res = "x" ]
        then
            res=1
        fi
   
        if [ $res = $cnt ]
        then
            #echo -n "  Please input the altibase package name (fullpath) : "
            STRING="  Please input the altibase package name (fullpath) : "
			get_string
            read res
            if [ x$res = "x" ]
            then
				echo
                echo "  [Error] You don't specify the altibase package name. "
                exit;
            else
                PKG_NAME=$res
            fi
        elif [ $res -gt $cnt -o $res -lt 1 ]
        then
            echo "  [Error] Wrong number."
            exit;
        else
            CMD="awk '{ if (NR == $res) print \$0}' $ALTI_PKG"
            PKG_NAME=`echo $CMD | sh`
        fi
	fi

		alti_home=`ls -l $ALTIBASE_HOME | head -1 | awk '{print $2}'`

		if [ $alti_home -eq 0 ]
		then
        	eval "\cp $PKG_NAME $ALTIBASE_HOME"
        	cd $ALTIBASE_HOME
        	$GZIP -cd $PKG_NAME | tar xvf -
		else
			echo
			echo "  $ALTIBASE_HOME directory is not empty."
			echo
			echo "  Of the following two options : "
			echo
			echo "   1) Copy & Unzip altibase package(overwriting)"
        	echo "   *) Already copy & unzip altibase package, so nothing"
			echo
			#echo -n "  Which would you like to perform? 1) "
			STRING="  Which would you like to perform? 1) "
			get_string
			read res
			if [ x$res = "x" ] || [ $res = 1 ]
			then
        		eval "\cp $PKG_NAME $ALTIBASE_HOME"
        		cd $ALTIBASE_HOME
        		$GZIP -cd $PKG_NAME | tar xvf -
			fi
		fi
    #fi
	
	echo > $CUR_SYSTEM_ENV
	echo > $REC_SYSTEM_ENV
	echo > $ALTI_PROFILE

	\rm $ALTI_PKG
}

chk_system ()
{
    if [ $SYSTEM = "HP-UX" ]
    then
        echo "  [ OS VERSION ]" | tee -a $CUR_SYSTEM_ENV
        uname -a | tee -a $CUR_SYSTEM_ENV
		echo | tee -a $CUR_SYSTEM_ENV

        echo "  [ PATCH LIST ]" | tee -a $CUR_SYSTEM_ENV
        /usr/sbin/swlist -l product | grep PH >> $CUR_SYSTEM_ENV
		echo | tee -a $CUR_SYSTEM_ENV
        /usr/sbin/swlist -l product | grep PH > $TMP 2>&1 
		chk_error

        echo "  [ CPU ]" | tee -a $CUR_SYSTEM_ENV
        ioscan -fnC processor | tee -a $CUR_SYSTEM_ENV
		echo | tee -a $CUR_SYSTEM_ENV
        ioscan -fnC processor > $TMP 2>&1
		chk_error

        echo "  [ MEMORY ]" | tee -a $CUR_SYSTEM_ENV
        grep Physical /var/adm/syslog/syslog.log | tee -a $CUR_SYSTEM_ENV
		echo | tee -a $CUR_SYSTEM_ENV
        grep -i Physical /var/adm/syslog/syslog.log > $TMP 2>&1
		chk_error

		if [ $result -eq 0 ]
		then
        	# MBytes
        	mem_size=`grep Physical /var/adm/syslog/syslog.log | awk '{printf("%d", $7/1024)}'`
		else
			mem_size=0
		fi

        echo "  [ SWAP ]" | tee -a $CUR_SYSTEM_ENV
		/etc/swapinfo -t | tee -a $CUR_SYSTEM_ENV
		echo | tee -a $CUR_SYSTEM_ENV
		/etc/swapinfo -t > $TMP 2>&1
		chk_error

		if [ $result -eq 0 ]
		then
        	# KBytes
        	swap_size=`/etc/swapinfo -t | grep dev | awk 'BEGIN {sum = 0} {sum += $2} END {printf("%d", sum)}'`
		else
			swap_size=0
		fi

        if [ $mem_size != 0 -a $swap_size != 0 ] && (( $mem_size > $swap_size ))
        then
            echo "  !!! [Error] The physical memory size is smaller than the swap size. !!!"
            exit;
        fi

        echo "  [ DISK ]" | tee -a $CUR_SYSTEM_ENV
        bdf | tee -a $CUR_SYSTEM_ENV
		echo | tee -a $CUR_SYSTEM_ENV
    elif [ $SYSTEM = "HP-RX" ]
    then
        echo "  [ MACHINE INFO ]" | tee -a $CUR_SYSTEM_ENV
        /usr/contrib/bin/machinfo | tee -a $CUR_SYSTEM_ENV
		echo | tee -a $CUR_SYSTEM_ENV
        /usr/contrib/bin/machinfo > $TMP 2>&1
		chk_error

        if [ $result -eq 0 ]
        then
            # MBytes
        	mem_size=`/usr/contrib/bin/machinfo | grep Memory | awk '{printf("%d", $3)}'`
        else
            mem_size=0
        fi

        echo "  [ PATCH LIST ]" | tee -a $CUR_SYSTEM_ENV
        /usr/sbin/swlist -l product | grep PH >> $CUR_SYSTEM_ENV
		echo | tee -a $CUR_SYSTEM_ENV
        /usr/sbin/swlist -l product | grep PH > $TMP 2>&1
		chk_error

        echo "  [ SWAP ]" | tee -a $CUR_SYSTEM_ENV
		/etc/swapinfo -t | tee -a $CUR_SYSTEM_ENV
		echo | tee -a $CUR_SYSTEM_ENV
		/etc/swapinfo -t > $TMP 2>&1
		chk_error

		if [ $result -eq 0 ]
		then
        	# MBytes
        	swap_size=`/etc/swapinfo -t | grep dev | awk 'BEGIN {sum = 0} {sum += $2} END {printf("%d", sum/1024)}'`
		else
			swap_size=0
		fi

        if [ $mem_size != 0 -a $swap_size != 0 ] && (( $mem_size > $swap_size ))
        then
            echo "  !!! [Error] The physical memory size is smaller than the swap size. !!!"
            exit;
        fi

        echo "  [ DISK ]" | tee -a $CUR_SYSTEM_ENV
        bdf | tee -a $CUR_SYSTEM_ENV
		echo | tee -a $CUR_SYSTEM_ENV
    elif [ $SYSTEM = "AIX" ]
    then
        echo "  [ OS VERSION ]" | tee -a $CUR_SYSTEM_ENV
        uname -a | tee -a $CUR_SYSTEM_ENV
		echo | tee -a $CUR_SYSTEM_ENV

        echo "  [ PATCH LIST ]" | tee -a $CUR_SYSTEM_ENV
        oslevel -r >> $CUR_SYSTEM_ENV
		echo | tee -a $CUR_SYSTEM_ENV
        instfix -ivq >> $CUR_SYSTEM_ENV
		echo | tee -a $CUR_SYSTEM_ENV
        oslevel -r > $TMP 2>&1
		chk_error
        instfix -ivq > $TMP 2>&1
		chk_error

        echo "  [ CPU ]" | tee -a $CUR_SYSTEM_ENV
        lsdev -Cc processor | tee -a $CUR_SYSTEM_ENV
		echo | tee -a $CUR_SYSTEM_ENV
        lsdev -Cc processor > $TMP 2>&1
		chk_error

        echo "  [ MEMORY ]" | tee -a $CUR_SYSTEM_ENV
        bootinfo -r | tee -a $CUR_SYSTEM_ENV
		echo | tee -a $CUR_SYSTEM_ENV
        bootinfo -r > $TMP 2>&1
		chk_error

        if [ $result -eq 0 ]
        then
            # MBytes
            mem_size=`bootinfo -r | awk '{printf("%d", $1/1024)}'`
        else
            mem_size=0
        fi

        echo "  [ SWAP ]" | tee -a $CUR_SYSTEM_ENV
		lsps -a | tee -a $CUR_SYSTEM_ENV
		echo | tee -a $CUR_SYSTEM_ENV
		lsps -a > $TMP 2>&1
		chk_error

        if [ $result -eq 0 ]
        then
            # MBytes
            swap_size=`lsps -a | grep -v Size | awk 'BEGIN {sum = 0} {sum += substr($4, 1, match($4, "MB")-1)} END {printf("%d", sum)}'`
        else
            swap_size=0
        fi

        if [ $mem_size != 0 -a $swap_size != 0 ] && (( $mem_size > $swap_size ))
        then
            echo "  !!! [Error] The physical memory size is smaller than the swap size. !!!"
            exit;
        fi

        echo "  [ DISK ]" | tee -a $CUR_SYSTEM_ENV
        df -k | tee -a $CUR_SYSTEM_ENV
		echo | tee -a $CUR_SYSTEM_ENV
    elif [ $SYSTEM = "SunOS" ]
    then
        echo "  [ OS VERSION ]" | tee -a $CUR_SYSTEM_ENV
        uname -a | tee -a $CUR_SYSTEM_ENV
		echo | tee -a $CUR_SYSTEM_ENV

        echo "  [ PATCH LIST ]" | tee -a $CUR_SYSTEM_ENV
        showrev -p >> $CUR_SYSTEM_ENV
		echo | tee -a $CUR_SYSTEM_ENV
        showrev -p > $TMP 2>&1
		chk_error

        echo "  [ CPU ]" | tee -a $CUR_SYSTEM_ENV
        /usr/sbin/psrinfo -v | tee -a $CUR_SYSTEM_ENV
		echo | tee -a $CUR_SYSTEM_ENV
        /usr/sbin/psrinfo -v > $TMP 2>&1
		chk_error

        echo "  [ MEMORY ]" | tee -a $CUR_SYSTEM_ENV
        prtconf | grep -i "memory size" | tee -a $CUR_SYSTEM_ENV
		echo | tee -a $CUR_SYSTEM_ENV
        prtconf | grep -i "memory size" > $TMP 2>&1
		chk_error

        if [ $result -eq 0 ]
        then
			# MBytes
			mem_size=`prtconf | grep -i "memory size" | awk '{printf("%d", $3)}'`
        else
            mem_size=0
        fi

        echo "  [ SWAP ]" | tee -a $CUR_SYSTEM_ENV
		swap -l | tee -a $CUR_SYSTEM_ENV
		echo | tee -a $CUR_SYSTEM_ENV
		swap -s | tee -a $CUR_SYSTEM_ENV
		echo | tee -a $CUR_SYSTEM_ENV
		swap -l > $TMP 2>&1
		chk_error
		swap -s > $TMP 2>&1
		chk_error

        if [ $result -eq 0 ]
        then
			# KBytes
			swap_used_size=`swap -s | awk '{printf("%d", $9)}' | tr -d "k"`
			swap_free_size=`swap -s | awk '{printf("%d", $11)}' | tr -d "k"`
			# MBytes
			swap_size=`expr \( $swap_used_size + $swap_free_size \) / 1024`
        else
            swap_size=0
        fi

        if [ $mem_size != 0 -a $swap_size != 0 ] && (( $mem_size > $swap_size ))
        then
            echo "  !!! [Error] The physical memory size is smaller than the swap size. !!!"
            exit;
        fi

        echo "  [ DISK ]" | tee -a $CUR_SYSTEM_ENV
        df -k | tee -a $CUR_SYSTEM_ENV
		echo | tee -a $CUR_SYSTEM_ENV
    elif [ $SYSTEM = "Linux" ]
    then
        echo "  [ OS VERSION ]" | tee -a $CUR_SYSTEM_ENV
        uname -a | tee -a $CUR_SYSTEM_ENV
        uname -m | tee -a $CUR_SYSTEM_ENV
        cat /etc/issue | tee -a $CUR_SYSTEM_ENV
		echo | tee -a $CUR_SYSTEM_ENV

        echo "  [ CPU ]" | tee -a $CUR_SYSTEM_ENV
        cat /proc/cpuinfo | tee -a $CUR_SYSTEM_ENV
		echo | tee -a $CUR_SYSTEM_ENV
        cat /proc/cpuinfo > $TMP 2>&1
		chk_error

        echo "  [ MEMORY ]" | tee -a $CUR_SYSTEM_ENV
        cat /proc/meminfo | tee -a $CUR_SYSTEM_ENV
		echo | tee -a $CUR_SYSTEM_ENV
        cat /proc/meminfo > $TMP 2>&1
		chk_error

        if [ $result -eq 0 ]
        then
			# MBytes
			mem_size=`cat /proc/meminfo | grep MemTotal | awk '{printf("%d", $3/1024)}'`
        else
            mem_size=0
        fi

        echo "  [ SWAP ]" | tee -a $CUR_SYSTEM_ENV
        cat /proc/swaps | tee -a $CUR_SYSTEM_ENV
		echo | tee -a $CUR_SYSTEM_ENV
        free -mt | tee -a $CUR_SYSTEM_ENV
		echo | tee -a $CUR_SYSTEM_ENV
        cat /proc/swaps > $TMP 2>&1
		chk_error
        free -mt > $TMP 2>&1
		chk_error

        if [ $result -eq 0 ]
        then
			# KBytes
            swap_size=`cat /proc/swaps | grep -v Size | awk 'BEGIN {sum = 0} {sum += $3} END {printf("%d", sum)}'`
        else
            swap_size=0
        fi

        if [ $mem_size != 0 -a $swap_size != 0 ] && (( $mem_size > $swap_size ))
        then
            echo "  !!! [Error] The physical memory size is smaller than the swap size. !!!"
            exit;
        fi

        echo "  [ DISK ]" | tee -a $CUR_SYSTEM_ENV
        df -k | tee -a $CUR_SYSTEM_ENV
		echo | tee -a $CUR_SYSTEM_ENV
    elif [ $SYSTEM = "OSF1" ]
    then
        echo "  [ OS VERSION ]" | tee -a $CUR_SYSTEM_ENV
        uname -a | tee -a $CUR_SYSTEM_ENV
		echo | tee -a $CUR_SYSTEM_ENV

        echo "  [ PATCH LIST ]" | tee -a $CUR_SYSTEM_ENV
        /usr/sbin/setld -i >> $CUR_SYSTEM_ENV
		echo | tee -a $CUR_SYSTEM_ENV
        /usr/sbin/setld -i > $TMP 2>&1
		chk_error

        echo "  [ CPU ]" | tee -a $CUR_SYSTEM_ENV
        /usr/sbin/psrinfo -v | tee -a $CUR_SYSTEM_ENV
		echo | tee -a $CUR_SYSTEM_ENV
        /usr/sbin/psrinfo -v > $TMP 2>&1
		chk_error

        echo "  [ MEMORY ]" | tee -a $CUR_SYSTEM_ENV
        /usr/sbin/uerf | grep memory | tee -a $CUR_SYSTEM_ENV
		echo | tee -a $CUR_SYSTEM_ENV
        /usr/sbin/uerf | grep memory > $TMP 2>&1
		chk_error

        if [ $result -eq 0 ]
        then
			/usr/sbin/uerf > $TMP &
			pid=$!
			sleep 3
			kill -9 $pid
			# MBytes
			mem_size=`grep "physical memory" $TMP | head -1 | awk '{printf("%d", $4)}'`
        else
            mem_size=0
        fi

        echo "  [ SWAP ]" | tee -a $CUR_SYSTEM_ENV
		swapon -s | tee -a $CUR_SYSTEM_ENV
		echo | tee -a $CUR_SYSTEM_ENV
		swapon -s > $TMP 2>&1
		chk_error

        if [ $result -eq 0 ]
        then
			# MBytes
			swap_size=`swapon -s | grep "Allocated space" | tail -1 | awk -v pgsize=`pagesize` '{printf("%.0f", $3*pgsize/1024/1024)}'`
        else
            swap_size=0
        fi

        if [ $mem_size != 0 -a $swap_size != 0 ] && (( $mem_size > $swap_size ))
        then
            echo "  !!! [Error] The physical memory size is smaller than the swap size. !!!"
            exit;
        fi

        echo "  [ DISK ]" | tee -a $CUR_SYSTEM_ENV
        df -k | tee -a $CUR_SYSTEM_ENV
		echo | tee -a $CUR_SYSTEM_ENV
    fi
}

chk_ker_para ()
{
    if [ $SYSTEM = "HP-UX" -o $SYSTEM = "HP-RX" ]
    then
		echo "  [ Current Kernel Parametere Values ] " | tee -a $CUR_SYSTEM_ENV
        eval "$KMTUNE | grep shmmax" | tee -a $CUR_SYSTEM_ENV
        eval "$KMTUNE | grep shmmni" | tee -a $CUR_SYSTEM_ENV
        eval "$KMTUNE | grep shmseg" | tee -a $CUR_SYSTEM_ENV
        eval "$KMTUNE | grep semmap" | tee -a $CUR_SYSTEM_ENV
        eval "$KMTUNE | grep semmni" | tee -a $CUR_SYSTEM_ENV
        eval "$KMTUNE | grep semmns" | tee -a $CUR_SYSTEM_ENV
        eval "$KMTUNE | grep semmnu" | tee -a $CUR_SYSTEM_ENV
        eval "$KMTUNE | grep semume" | tee -a $CUR_SYSTEM_ENV
        eval "$KMTUNE | grep max_thread_proc" | tee -a $CUR_SYSTEM_ENV
        eval "$KMTUNE | grep maxusers" | tee -a $CUR_SYSTEM_ENV
        eval "$KMTUNE | grep maxdsiz" | tee -a $CUR_SYSTEM_ENV
		echo | tee -a $CUR_SYSTEM_ENV

        echo "  [ Recommand Kernel Parametere Values ] " | tee -a $REC_SYSTEM_ENV
        echo "  shmmax = 2147483648" | tee -a $REC_SYSTEM_ENV
        echo "  shmmni = 500" | tee -a $REC_SYSTEM_ENV
        echo "  shmseg = 200" | tee -a $REC_SYSTEM_ENV
        echo "  semmap = 1001" | tee -a $REC_SYSTEM_ENV
        echo "  semmni = 1000" | tee -a $REC_SYSTEM_ENV
        echo "  semmns = 4096" | tee -a $REC_SYSTEM_ENV
        echo "  semmnu = 1000" | tee -a $REC_SYSTEM_ENV
        echo "  semume = 1000" | tee -a $REC_SYSTEM_ENV
        echo "  max_thread_proc = 600" | tee -a $REC_SYSTEM_ENV
        echo "  maxusers = 64" | tee -a $REC_SYSTEM_ENV
        echo "  maxdsiz = 1073741824" | tee -a $REC_SYSTEM_ENV
		#mem_size=`expr $mem_size \* 1024 \* 1024`
        #echo "  maxdsiz_64bit = $mem_size" | tee -a $REC_SYSTEM_ENV
        echo "  maxdsiz_64bit = 4294967296" | tee -a $REC_SYSTEM_ENV
        echo | tee -a $REC_SYSTEM_ENV

        echo "  [ How to modify kernel parameter values ] " | tee -a $REC_SYSTEM_ENV
        echo "  $KMTUNE -s shmmax=2147483648" | tee -a $REC_SYSTEM_ENV
        echo "  $KMTUNE -s shmmni=500" | tee -a $REC_SYSTEM_ENV
        echo "  $KMTUNE -s shmseg=200" | tee -a $REC_SYSTEM_ENV
        echo "  $KMTUNE -s semmap=1001" | tee -a $REC_SYSTEM_ENV
        echo "  $KMTUNE -s semmni=1000" | tee -a $REC_SYSTEM_ENV
        echo "  $KMTUNE -s semmns=4096" | tee -a $REC_SYSTEM_ENV
        echo "  $KMTUNE -s semmnu=1000" | tee -a $REC_SYSTEM_ENV
        echo "  $KMTUNE -s semume=1000" | tee -a $REC_SYSTEM_ENV
        echo "  $KMTUNE -s max_thread_proc=600" | tee -a $REC_SYSTEM_ENV
        echo "  $KMTUNE -s maxusers=64" | tee -a $REC_SYSTEM_ENV
        echo "  $KMTUNE -s maxdsiz=1073741824" | tee -a $REC_SYSTEM_ENV
        #echo "  $KMTUNE -s maxdsiz_64bit=$mem_size" | tee -a $REC_SYSTEM_ENV
        echo "  $KMTUNE -s maxdsiz_64bit=4294967296" | tee -a $REC_SYSTEM_ENV
		echo | tee -a $REC_SYSTEM_ENV
           
        maxdsiz_64bit=`$KMTUNE | grep maxdsiz_64bit | awk '{print $2}'`
        if (( $maxdsiz_64bit < 4294967296 ))
        then
            echo "  !!! [Warning] The maxdsiz_64bit value must be which is 4G. !!!" | tee -a $INSTALL_RESULT
            echo | tee -a $INSTALL_RESULT
		elif (( $maxdsiz_64bit < 2147483648 ))
		then
            echo "  !!! [Error] The maxdsiz_64bit value must be which is at least 2G. !!!"
			exit;
        fi
    elif [ $SYSTEM = "AIX" ]
    then
        echo "  [ Recommand Kernel Parametere Values (/etc/security/limits - default:) ] " | tee -a $REC_SYSTEM_ENV
        echo "  fsize = -1" | tee -a $REC_SYSTEM_ENV
        echo "  data = -1" | tee -a $REC_SYSTEM_ENV
        echo "  rss = -1" | tee -a $REC_SYSTEM_ENV
		echo | tee -a $REC_SYSTEM_ENV

        echo "  [ How to modify kernel parameter values ] " | tee -a $REC_SYSTEM_ENV
		echo "  Edit /etc/security/limits file." | tee -a $REC_SYSTEM_ENV
		echo | tee -a $REC_SYSTEM_ENV
    elif [ $SYSTEM = "SunOS" ]
    then
        echo "  [ Current Kernel Parametere Values ] " | tee -a $CUR_SYSTEM_ENV
        eval "sysdef -i | grep SHM" | tee -a $CUR_SYSTEM_ENV
        eval "sysdef -i | grep SEM" | tee -a $CUR_SYSTEM_ENV
		echo "rlim_fd_max/D" | adb -k | sed -n '3,3p' | tee -a $CUR_SYSTEM_ENV
		echo "rlim_fd_cur/D" | adb -k | sed -n '3,3p' | tee -a $CUR_SYSTEM_ENV
        echo | tee -a $CUR_SYSTEM_ENV

		adb -k
		if [ $? -eq 0 ]
		then 
			FD_HARD_LIMIT=`echo "rlim_fd_max/D" | adb -k | sed -n '3,3p' | awk '{print $2}'`
	
			if [ $FD_HARD_LIMIT -lt 1024 ]
			then
	    		echo "  !!! [Error] rlim_fd_max value must be 1024. !!!"
				exit;
			elif [ $FD_HARD_LIMIT -lt 2048 ]
			then
	    		echo "  !!! [Warning] rlim_fd_max value must be 2048. !!!" | tee -a $INSTALL_RESULT
				echo | tee -a $INSTALL_RESULT
			fi
		fi

        echo "  [ Recommand Kernel Parameter Values ] " | tee -a $REC_SYSTEM_ENV
        echo "  shminfo_shmmax = 2147483648" | tee -a $REC_SYSTEM_ENV
        echo "  shminfo_shmmin = 1" | tee -a $REC_SYSTEM_ENV
        echo "  shminfo_shmmni = 500" | tee -a $REC_SYSTEM_ENV
        echo "  shminfo_shmseg = 200" | tee -a $REC_SYSTEM_ENV
        echo "  seminfo_semmns = 8192" | tee -a $REC_SYSTEM_ENV
        echo "  seminfo_semmni = 5029" | tee -a $REC_SYSTEM_ENV
        echo "  seminfo_semmsl = 2000" | tee -a $REC_SYSTEM_ENV
        echo "  seminfo_semmap = 5024" | tee -a $REC_SYSTEM_ENV
        echo "  seminfo_semmnu = 1024" | tee -a $REC_SYSTEM_ENV
        echo "  seminfo_semopm = 512" | tee -a $REC_SYSTEM_ENV
        echo "  seminfo_semume = 512" | tee -a $REC_SYSTEM_ENV
        echo "  rlim_fd_max = 4096" | tee -a $REC_SYSTEM_ENV
        echo "  rlim_fd_cur = 2048" | tee -a $REC_SYSTEM_ENV
        echo | tee -a $REC_SYSTEM_ENV

        echo "  [ How to modify kernel parameter values ] " | tee -a $REC_SYSTEM_ENV
		echo "  Edit /etc/system file." | tee -a $REC_SYSTEM_ENV
        echo "  set shmsys:shminfo_shmmax = 2147483648" | tee -a $REC_SYSTEM_ENV
        echo "  set shmsys:shminfo_shmmin = 1" | tee -a $REC_SYSTEM_ENV
        echo "  set shmsys:shminfo_shmmni = 500" | tee -a $REC_SYSTEM_ENV
        echo "  set shmsys:shminfo_shmseg = 200" | tee -a $REC_SYSTEM_ENV
        echo "  set semsys:seminfo_semmns = 8192" | tee -a $REC_SYSTEM_ENV
        echo "  set semsys:seminfo_semmni = 5029" | tee -a $REC_SYSTEM_ENV
        echo "  set semsys:seminfo_semmsl = 2000" | tee -a $REC_SYSTEM_ENV
        echo "  set semsys:seminfo_semmap = 5024" | tee -a $REC_SYSTEM_ENV
        echo "  set semsys:seminfo_semmnu = 1024" | tee -a $REC_SYSTEM_ENV
        echo "  set semsys:seminfo_semopm = 512" | tee -a $REC_SYSTEM_ENV
        echo "  set semsys:seminfo_semume = 512" | tee -a $REC_SYSTEM_ENV
        echo "  set rlim_fd_max = 4096" | tee -a $REC_SYSTEM_ENV
        echo "  set rlim_fd_cur = 2048" | tee -a $REC_SYSTEM_ENV
		echo | tee -a $REC_SYSTEM_ENV
    elif [ $SYSTEM = "Linux" ]
    then
        echo "  [ How to modify kernel parameter values ] " | tee -a $REC_SYSTEM_ENV
        echo "  echo 2147483648 > /proc/sys/kernel/shmmax" | tee -a $REC_SYSTEM_ENV
        echo "  echo 512 32000 512 512 > /proc/sys/kernel/sem" | tee -a $REC_SYSTEM_ENV
        echo | tee -a $REC_SYSTEM_ENV
    elif [ $SYSTEM = "OSF1" ]
    then
		echo "  [ Current Kernel Parametere Values ] " | tee -a $CUR_SYSTEM_ENV
        /sbin/sysconfig -q ipc | grep shm | tee -a $CUR_SYSTEM_ENV
        /sbin/sysconfig -q ipc | grep sem | tee -a $CUR_SYSTEM_ENV
        /sbin/sysconfig -q proc | grep per_proc_address | tee -a $CUR_SYSTEM_ENV
        /sbin/sysconfig -q proc | grep per_proc_data | tee -a $CUR_SYSTEM_ENV
        /sbin/sysconfig -q proc | grep thread | tee -a $CUR_SYSTEM_ENV
        /sbin/sysconfig -q vm | grep mapentries | tee -a $CUR_SYSTEM_ENV
        /sbin/sysconfig -q vm | grep vpagemax | tee -a $CUR_SYSTEM_ENV
        echo | tee -a $CUR_SYSTEM_ENV

        echo "  [ Recommand Kernel Parameter Values ] " | tee -a $REC_SYSTEM_ENV
        echo "  shm-max = 2147483648" | tee -a $REC_SYSTEM_ENV
        echo "  shm_min = 1" | tee -a $REC_SYSTEM_ENV
        echo "  shm_mni = 500" | tee -a $REC_SYSTEM_ENV
        echo "  shm_seg = 512" | tee -a $REC_SYSTEM_ENV
        echo "  sem-mni = 1024" | tee -a $REC_SYSTEM_ENV
        echo "  sem-msl = 512" | tee -a $REC_SYSTEM_ENV
        echo "  sem-opm = 512" | tee -a $REC_SYSTEM_ENV
        echo "  sem-ume = 512" | tee -a $REC_SYSTEM_ENV
        echo "  sem-vmx = 70000" | tee -a $REC_SYSTEM_ENV
        echo "  sem-aem = 16384" | tee -a $REC_SYSTEM_ENV
        echo "  max-threads-per-user = 1024" | tee -a $REC_SYSTEM_ENV
        echo "  thread-max = 2048" | tee -a $REC_SYSTEM_ENV
        echo "  mapentries = 400" | tee -a $REC_SYSTEM_ENV
        echo "  per-proc-address-space = 4294967296" | tee -a $REC_SYSTEM_ENV
        echo "  max-per-proc-address-space = 4294967296" | tee -a $REC_SYSTEM_ENV
        echo "  max-per-proc-data-size = 4294967296" | tee -a $REC_SYSTEM_ENV
        echo "  per-proc-data-size = 4294967296" | tee -a $REC_SYSTEM_ENV
        #echo "  per-proc-address-space = $mem_size" | tee -a $REC_SYSTEM_ENV
        #echo "  max-per-proc-address-space = $mem_size" | tee -a $REC_SYSTEM_ENV
        #echo "  max-per-proc-data-size = $mem_size" | tee -a $REC_SYSTEM_ENV
        #echo "  per-proc-data-size = $mem_size" | tee -a $REC_SYSTEM_ENV

        /usr/sbin/uerf -r 300 | grep "physical memory" 2>&1 /tmp/tmp.txt &
       	kill -9 $!
       	mem_size=`head -1 /tmp/tmp.txt | awk '{print $4}'`
       	vpagemax=`expr \( $mem_size \* 1024 \* 1024 \) / 8192`
       	echo "  vm-vpagemax = $vpagemax" | tee -a $REC_SYSTEM_ENV
		echo | tee -a $REC_SYSTEM_ENV

		max_address=`/sbin/sysconfig -q proc | grep max_per_proc_address_space`
		per_address=`/sbin/sysconfig -q proc | grep per_proc_address_space`
		max_data=`/sbin/sysconfig -q proc | grep max_per_proc_data_size`
		per_data=`/sbin/sysconfig -q proc | grep per_proc_data_size`

        if [ $max_address < "4294967296" ]
        then
            echo "  !!! [Warning] The max_per_proc_address_space value must be which is 4G. !!!" | tee -a $INSTALL_RESULT
            echo | tee -a $INSTALL_RESULT
		elif [ $max_address < "2147483648" ]
		then
            echo "  !!! [Error] The max_per_proc_address_space value must be which is at least 2G. !!!"
			exit;
        fi

        if [ $per_address < "4294967296" ]
        then
            echo "  !!! [Warning] The per_proc_address_space value must be which is 4G. !!!" | tee -a $INSTALL_RESULT
            echo | tee -a $INSTALL_RESULT
		elif [ $per_address < "2147483648" ]
		then
            echo "  !!! [Error] The per_proc_address_space value must be which is at least 2G. !!!"
			exit;
        fi

        if [ $max_data < "4294967296" ]
        then
            echo "  !!! [Warning] The max_per_proc_data_size value must be which is 4G. !!!" | tee -a $INSTALL_RESULT
            echo | tee -a $INSTALL_RESULT
		elif [ $max_data < "2147483648" ]
		then
            echo "  !!! [Error] The max_per_proc_data_size value must be which is at least 2G. !!!"
			exit;
        fi

        if [ $per_data < "4294967296" ]
        then
            echo "  !!! [Warning] The per_proc_data_size value must be which is 4G. !!!" | tee -a $INSTALL_RESULT
            echo | tee -a $INSTALL_RESULT
		elif [ $per_data < "2147483648" ]
		then
            echo "  !!! [Error] The per_proc_data_size value must be which is at least 2G. !!!"
			exit;
        fi
    fi

	echo "  !!! You should modify kernel parameters and reboot the system to apply new kernel parameters. !!!"
}

set_env ()
{
    cd $HOME
    mysh=`env | grep SHELL | tr -d "SHELL="`
    mysh=`basename $mysh`

    case $mysh in
    csh)
        if [ -f .cshrc ]
        then
            profile=".cshrc"
        elif [ -f .login ]
        then
            profile=".login"
        else
            profile=".cshrc"
        fi;;
    bash)
        if [ -f .bash_profile ]
        then
            profile=".bash_profile"
        elif [ -f .bash_login ]
        then
            profile=".bash_login"
        elif [ -f .profile ]
        then
            profile=".profile"
        else
            profile=".bash_profile"
        fi;;
    ksh)
        if [ -f .profile ]
        then
            profile=".profile"
        elif [ -f .kshrc ]
        then
            profile=".kshrc"
        else
            profile=".profile"
        fi;;
    sh)
        profile=".profile";;
    *)
        echo "  Sorry, you have to set above enveroment variables in your shell init file yourself."
        return;;
    esac

    echo "  [ ulimit -a ] " | tee -a $CUR_SYSTEM_ENV
    eval "ulimit -a" | tee -a $CUR_SYSTEM_ENV
    echo | tee -a $CUR_SYSTEM_ENV

	FD_HARD_LIMIT=`ulimit -Hn`

	if [ $FD_HARD_LIMIT -lt 1024 ]
	then
   		echo "  !!! [Error] file descriptor hard limit value must be 1024. !!!"
		exit;
	elif [ $FD_HARD_LIMIT -lt 2048 ]
	then
   		echo "  !!! [Warning] file descriptor hard limit value must be 2048. !!!" | tee -a $INSTALL_RESULT
		echo | tee -a $INSTALL_RESULT
		OPEN_FILES=$FD_HARD_LIMIT
	fi

   	#echo -n "  Please specify file descriptor limit ($OPEN_FILES) : "
   	STRING="  Please specify file descriptor limit ($OPEN_FILES) : "
	get_string
    read open_files
    if [ x$open_files = "x" ]
    then
        open_files=$OPEN_FILES
    fi

    echo "" >> $ALTI_PROFILE
    echo "# For Altibase" >> $ALTI_PROFILE

    case $mysh in
    csh)
        echo "setenv ALTIBASE_HOME $ALTIBASE_HOME" >> $ALTI_PROFILE
        echo "setenv PATH \$ALTIBASE_HOME/bin:\$PATH" >> $ALTI_PROFILE
        echo "setenv LD_LIBRARY_PATH \$ALTIBASE_HOME/lib:\$LD_LIBRARY_PATH" >> $ALTI_PROFILE
        echo "setenv CLASSPATH \$ALTIBASE_HOME/lib/Altibase.jar:\$CLASSPATH" >> $ALTI_PROFILE
        if [ $SYSTEM = "SunOS" ]
        then
            echo "setenv LD_LIBRARY_PATH_64 \$ALTIBASE_HOME/lib:\$LD_LIBRARY_PATH_64" >> $ALTI_PROFILE
        elif [ $SYSTEM = "HP-UX" -a $SYSTEM = "HP-RX" ]
        then
            echo "setenv SHLIB_PATH \$ALTIBASE_HOME/lib:\$SHLIB_PATH" >> $ALTI_PROFILE
#           echo >> $ALTI_PROFILE
            echo "setenv PTHREAD_DISABLE_HANDOFF ON" >> $ALTI_PROFILE
#           echo "setenv PTHREAD_FORCE_SCOPE_SYSTEM 1 >> $ALTI_PROFILE
#           echo "setenv PERF_ENABLE 1 >> $ALTI_PROFILE
#           echo "setenv _M_ARENA_OPTS 20:32 >> $ALTI_PROFILE
        elif [ $SYSTEM = "AIX" ]
        then
            echo "setenv AIXTHREAD_MUTEX_DEBUG OFF" >> $ALTI_PROFILE
            echo "setenv AIXTHREAD_RWLOCK_DEBUG OFF" >> $ALTI_PROFILE
            echo "setenv AIXTHREAD_COND_DEBUG OFF" >> $ALTI_PROFILE
            echo "setenv AIXTHREAD_MNRATIO 1:1" >> $ALTI_PROFILE
            echo "setenv AIXTHREAD_SCOPE S" >> $ALTI_PROFILE
            echo "setenv MALLOCMULTIHEAP 1" >> $ALTI_PROFILE
        fi

        echo "" >> $ALTI_PROFILE
        echo "limit datasize unlimited" >> $ALTI_PROFILE
        echo "limit filesize unlimited" >> $ALTI_PROFILE
        echo "limit descriptors $open_files" >> $ALTI_PROFILE
		echo "" >> $ALTI_PROFILE

		source $ALTI_PROFILE;;
    *)
        echo "ALTIBASE_HOME=$ALTIBASE_HOME; export ALTIBASE_HOME" >> $ALTI_PROFILE
        echo "PATH=\$ALTIBASE_HOME/bin:\$PATH; export PATH" >> $ALTI_PROFILE
        echo "LD_LIBRARY_PATH=\$ALTIBASE_HOME/lib:\$LD_LIBRARY_PATH; export LD_LIBRARY_PATH" >> $ALTI_PROFILE
        echo "CLASSPATH=\$ALTIBASE_HOME/lib/Altibase.jar:\$CLASSPATH; export CLASSPATH" >> $ALTI_PROFILE
        if [ $SYSTEM = "SunOS" ]
        then
            echo "LD_LIBRARY_PATH_64=\$ALTIBASE_HOME/lib:\$LD_LIBRARY_PATH_64; export LD_LIBRARY_PATH_64" >> $ALTI_PROFILE
        elif [ $SYSTEM = "HP-UX" -o $SYSTEM = "HP-RX" ]
        then
            echo "SHLIB_PATH=\$ALTIBASE_HOME/lib:\$SHLIB_PATH; export SHLIB_PATH" >> $ALTI_PROFILE
#           echo >> $ALTI_PROFILE
            echo "PTHREAD_DISABLE_HANDOFF=ON; export PTHREAD_DISABLE_HANDOFF" >> $ALTI_PROFILE
#           echo "PTHREAD_FORCE_SCOPE_SYSTEM=1; export PTHREAD_FORCE_SCOPE_SYSTEM >> $ALTI_PROFILE
#           echo "PERF_ENABLE=1; export PERF_ENABLE >> $ALTI_PROFILE
#           echo "_M_ARENA_OPTS=20:32; export _M_ARENA_OPTS >> $ALTI_PROFILE
        elif [ $SYSTEM = "AIX" ]
        then
            echo "AIXTHREAD_MUTEX_DEBUG=OFF; export AIXTHREAD_MUTEX_DEBUG" >> $ALTI_PROFILE
            echo "AIXTHREAD_RWLOCK_DEBUG=OFF; export AIXTHREAD_RWLOCK_DEBUG" >> $ALTI_PROFILE
            echo "AIXTHREAD_COND_DEBUG=OFF; export AIXTHREAD_COND_DEBUG" >> $ALTI_PROFILE
            echo "AIXTHREAD_MNRATIO=1:1; export AIXTHREAD_MNRATIO" >> $ALTI_PROFILE
            echo "AIXTHREAD_SCOPE=S; export AIXTHREAD_SCOPE" >> $ALTI_PROFILE
            echo "MALLOCMULTIHEAP=1; export MALLOCMULTIHEAP" >> $ALTI_PROFILE
        fi

        echo "" >> $ALTI_PROFILE
        echo "ulimit -d unlimited" >> $ALTI_PROFILE
        echo "ulimit -f unlimited" >> $ALTI_PROFILE
        echo "ulimit -n $open_files" >> $ALTI_PROFILE
		echo "" >> $ALTI_PROFILE

		. $ALTI_PROFILE;;
    esac

	echo
    echo "================================================================="
    cat $ALTI_PROFILE
    echo "================================================================="
	echo

    echo "  SHELL = $mysh"
	echo
    echo "  [ Shell Init File ] " | tee -a $CUR_SYSTEM_ENV
    echo "  $profile" | tee -a $CUR_SYSTEM_ENV
    echo "" | tee -a $CUR_SYSTEM_ENV

	cp $profile $ORG_PROFILE

    #echo -n "  Would you like to append upper enviroment variables into $profile? (yes) "
    STRING="  Would you like to append upper enviroment variables into $profile? (yes) "
	get_string
    read res
    if [ x$res = "x" ]
    then
            cat $ALTI_PROFILE >> $profile
    elif [ $res = "yes" -o $res = "y" -o $res = "YES" -o $res = "Y" ]
    then
            cat $ALTI_PROFILE >> $profile
    fi
}

set_license ()
{
    echo "  How to get the license is following."
    echo
    echo "  1) Connect to 'http://www.altibase.com'."
    echo "  2) Move into 'Download Center'."
    echo "  3) Click 'Get License'."
    echo "  4) Fill out the Form and then click 'confirm'."
    echo

	if [ $SYSTEM = "HP-UX" -o $SYSTEM = "HP-RX" ]
	then
		hostid=`uname -a | awk '{printf("%08x", $6)}'`
	else
		hostid=`hostid`
	fi
    echo "  Your hostid = $hostid"
	echo

    #echo -n "  Please input the license key : "
    STRING="  Please input the license key : "
	get_string
    read license
    echo $license >> $ALTIBASE_HOME/conf/license
}

get_property ()
{
    echo
    STRING="  Database Name (mydb) : "
        get_string
    read res
    if [ x$res != "x" ]
    then
                DB_NAME=$res
    fi

    echo
    #echo -n "  Memory Database File Directory ($ALTIBASE_HOME/dbs) : "
    STRING="  Memory Database File Directory ($ALTIBASE_HOME/dbs) : "
	get_string
    read res
    if [ x$res != "x" ]
    then
		MEM_DB_DIR=$res
    fi

    #echo -n "  Max Memory DB Size (4G) : "
    STRING="  Max Memory DB Size (4G) : "
	get_string
    read MEM_MAX_DB_SIZE
    if [ x$MEM_MAX_DB_SIZE = "x" ]
    then
        MEM_MAX_DB_SIZE=4G
    fi

    echo
    echo "  Buffer Pool Size for Disk Tables"
    #echo -n "  Do you have any plan use Disk Tables? (yes) "
    STRING="  Do you have any plan use Disk Tables? (yes) "
	get_string
    read res
    if [ x$res = "x" ] || [ $res = "Y" -o $res = "y" -o $res = "YES" -o $res = "yes" ]
    then
        #echo -n "  It's in mbytes. Please input digit only for buffer pool size (default 130) : "
        STRING="  It's in mbytes. Please input digit only for buffer pool size (default 130) : "
		get_string
        read BUFFER_POOL_SIZE
        if [ x$BUFFER_POOL_SIZE = "x" ]
        then
            BUFFER_POOL_SIZE=16384
        else
            BUFFER_POOL_SIZE=`expr $BUFFER_POOL_SIZE \* 1024 \* 1024 / 8192`
        fi
    else
        BUFFER_POOL_SIZE=16
    fi

    echo
    echo "  Of the following kind of NLS_USE : "
    echo
    echo "  1) US7ASCII"
    echo "  2) KO16KSC5601"
    echo "  3) ZHT16BIG5"
    echo "  4) ZHS16CGB231280"
    echo "  5) ZHS16GBK"
    echo "  6) SHIFTJIS"
    echo "  7) MS932"
    echo "  8) UTF8"
    echo
    #echo -n "  Please choose one ? 1) "
    STRING="  Please choose one ? 1) "
	get_string
    read nls_use
    if [ x$nls_use = "x" ]
    then
        nls_use=1
    fi

    case $nls_use in
    1)
        NLS_USE=US7ASCII;;
    2)
        NLS_USE=KO16KSC5601;;
    3)
        NLS_USE=ZHT16BIG5;;
    4)
        NLS_USE=ZHS16CGB231280;;
    5)
        NLS_USE=ZHS16GBK;;
    6)
        NLS_USE=SHIFTJIS;;
    7)
        NLS_USE=MS932;;
    8)
        NLS_USE=UTF8;;
    *)
        NLS_USE=US7ASCII;;
    esac

    echo
    echo "  !!! Please separate datafiles storage and logfiles storage, it recommends. !!!"
}

set_property ()
{
    echo
    echo "  Creating altibase.propertyes file ..."


    i=0
    cd $ALTIBASE_HOME/conf
    echo "" > prop.tmp
    cat altibase.properties | while read val
    do
        prop_name=`echo $val | awk '{print $1}'`
        if [ x$prop_name = "x" ]
        then
            echo >> prop.tmp
        else
            tmp=`echo "$val" | grep -E "^[ ]*#" | wc -l`
            #if [ ${prop_name:0:1} = "#" ]
            if [ $tmp != "0" ]
            then
                echo "$val" >> prop.tmp
                continue;
            fi

            case $prop_name in
            DB_NAME)
                echo "DB_NAME = $DB_NAME" >> prop.tmp;;
            MEM_DB_DIR)
                echo "MEM_DB_DIR = $MEM_DB_DIR" >> prop.tmp;
                echo "MEM_DB_DIR = $MEM_DB_DIR" >> prop.tmp;;
            MEM_MAX_DB_SIZE)
                echo "MEM_MAX_DB_SIZE = $MEM_MAX_DB_SIZE" >> prop.tmp;;
            BUFFER_POOL_SIZE)
                echo "BUFFER_POOL_SIZE = $BUFFER_POOL_SIZE" >> prop.tmp;;
            NLS_USE)
                echo "NLS_USE = $NLS_USE" >> prop.tmp;;
            *)
                echo $val >> prop.tmp;;
            esac
        fi
    done

    cp altibase.properties altibase.properties.org
    mv prop.tmp altibase.properties
}

crt_db ()
{
	dbs=`ls -l $ALTIBASE_HOME/dbs | head -1 | awk '{print $2}'`
	logs=`ls -l $ALTIBASE_HOME/logs | head -1 | awk '{print $2}'`
	dbs2=$dbs

	if [ $MEM_DB_DIR != "?/dbs" ]
	then
		dbs2=`ls -l $MEM_DB_DIR | head -1 | awk '{print $2}'`
	else
		MEM_DB_DIR="$ALTIBASE_HOME/dbs"
	fi

	if [ $dbs = "0" ] && [ $logs = "0" ] && [ $dbs2 = "0" ]
	then
		#echo -n "Would you like to create new database? (yes) "
		STRING="Would you like to create new database? (yes) "
		get_string
		read res
    	if [ x$res = "x" ] || [ $res = "Y" -o $res = "y" -o $res = "YES" -o $res = "yes" ]
		then
			crt_db2
		fi
	else
		echo "  $ALTIBASE_HOME/dbs or $MEM_DB_DIR or $ALTIBASE_HOME/logs directory is not empty."
		echo
		echo "  Of the following two options : "
		echo
		echo "   1) Use current database"
		echo "   *) Destroy & Create database"
		echo
		#echo -n "  Which would you like to perform? 1) "
		STRING="  Which would you like to perform? 1) "
		get_string
		read res
		if [ x$res != "x" ] && [ $res != 1 ]
		then
            eval "\rm $ALTIBASE_HOME/dbs/*"
            eval "\rm $MEM_DB_DIR/*"
            eval "\rm $ALTIBASE_HOME/logs/*"
		    crt_db2
		fi
    fi
}

crt_db2 ()
{
    echo
    #echo -n "Database Name (mydb) : "
    #STRING="Database Name (mydb) : "
	#get_string
    #read dbname
    #if [ x$dbname = "x" ]
    #then
    #    dbname=mydb
    #fi

#    echo -n "Memory DB InitSize (10M) : "
#    read initsize
#    if [ x$initsize = "x" ]
#    then
#       initsize=10M
#    fi

    initsize=10M
    filesize=1G

    #echo -n "Archive Log Mode (noarchivelog) : "
    STRING="Archive Log Mode (noarchivelog) : "
	get_string
    read arch
    if [ x$arch = "x" ]
    then
        arch=noarchivelog
    fi

    ${ADMIN} <<EOF
    startup process;
    create database $DB_NAME INITSIZE=$initsize $arch;
    shutdown abort
    quit
EOF
}


##############################
# altibase_install.sh start
##############################

echo
echo "Altibase Installation Start!!"
echo
	
if [ ! -f $INSTALL_STEP ]
then
	echo "<<< OS Check >>>"
	
	chk_os

	echo "1" > $INSTALL_STEP
else
	echo "  Of the following two options : "
	echo
	echo "   1) Keep installing"
	echo "   *) Install from the beginning"
	echo
	#echo -n "  Which would you like to perform? 1) "
	STRING="  Which would you like to perform? 1) "
	get_string
	read res
	if [ x$res != "x" ] && [ $res != 1 ]
	then
		echo
		echo "<<< OS Check >>>"
		
		chk_os
	
		echo "1" > $INSTALL_STEP
	fi
fi
	
while :
do
	step=`cat $INSTALL_STEP`

    case $step in
    1)
		echo
		#echo -n "  Altibase user is $WHOAMI. Is right? (yes) "
		STRING="  Altibase user is $WHOAMI. Is right? (yes) "
		get_string
		read res
		if [ x$res != "x" ]
		then
		    if [ $res != "Y" -a $res != "y" -a $res != "YES" -a $res != "yes" ]
		    then
		        echo "  [Error] Please try altibase_install.sh after login altibase user."
		        exit;
		    fi
		fi
		
		echo
		echo "  Of the following two options : "
		echo
		echo "   1) New install"
		echo "   *) Patch or Upgrade"
		echo
		#echo -n "  Which would you like to perform? 1) "
		STRING="  Which would you like to perform? 1) "
		get_string
		read install_option
		if [ x$install_option = "x" ] || [ $install_option = 1 ]
		then
		    echo
		    echo "<<< Install Altibase Package >>>"
		    echo
		
		    install_pkg
		
			echo "2" > $INSTALL_STEP
		fi

		step=`expr $step + 1`
		continue;;
    2)
		if [ x$install_option = "x" ] || [ $install_option = 1 ]
		then
		    echo
		    echo "<<< System Enviroment Check >>>"
		    echo
		
		    chk_system
		
			echo "3" > $INSTALL_STEP
		fi

		step=`expr $step + 1`
		continue;;
    3)
		if [ x$install_option = "x" ] || [ $install_option = 1 ]
		then
		    echo
		    echo "<<< Kernel Parameteres Check >>>"
		    echo
		
		    chk_ker_para
		
			echo "4" > $INSTALL_STEP
		fi

		step=`expr $step + 1`
		continue;;
    4)
		if [ x$install_option = "x" ] || [ $install_option = 1 ]
		then
		    echo
		    echo "<<< Enviroment Variables Setting >>>"
		    echo
		
		    set_env
		
			echo "5" > $INSTALL_STEP
		fi

		step=`expr $step + 1`
		continue;;
    5)
		if [ x$install_option = "x" ] || [ $install_option = 1 ]
		then
			echo "5" > $INSTALL_STEP
		
		    echo
		    echo "<<< License Setting >>>"
		    echo
		
		    set_license
		
			echo "6" > $INSTALL_STEP
		fi

		step=`expr $step + 1`
		continue;;
    6)
		echo
		echo "<<< Altibase Properties Setting >>>"
		echo
		
                cd $ALTIBASE_HOME/conf
                cp altibase.properties.sample altibase.properties

		echo "  Of the following three options : "
		echo
		echo "   1) Use default altibase property file"
		echo "   *) Edit altibase property file"
		#echo "   *) Use current altibase property file"
		echo
		#echo -n "  Which would you like to perform? 1) "
		STRING="  Which would you like to perform? 1) "
		get_string
		read res
		if [ x$res != "x" ] && [ $res = 2 ]
		then
		    get_property
		    set_property
		fi
		
		echo "7" > $INSTALL_STEP

		step=`expr $step + 1`
		continue;;
    7)
		echo
		echo "<<< Database Creation >>>"
		echo
		
		crt_db
		
		echo "8" > $INSTALL_STEP

		step=`expr $step + 1`
		continue;;
    8)
		echo
		echo "<<< Startup Altibase >>>"
		echo
		
		#echo -n "Would you like to altibase startup? (yes) "
		STRING="Would you like to altibase startup? (yes) "
		get_string
		read res
		if [ x$res = "x" ]
		then
			alti=`ps -ef | grep "altibase -p" | grep -v grep | grep $WHOAMI | awk '{print $2}'` 
			if [ x$alti = "x" ]
			then
		    	eval "server start"
			else
            	echo "  !!! Altibase server is already running now. !!!"
			fi
		elif [ $res = "Y" -o $res = "y" -o $res = "YES" -o $res = "yes" ]
		then
			alti=`ps -ef | grep "altibase -p" | grep -v grep | grep $WHOAMI | awk '{print $2}'` 
			if [ x$alti = "x" ]
			then
		    	eval "server start"
			else
            	echo "  !!! Altibase server is already running now. !!!"
			fi
		fi
		
		echo "9" > $INSTALL_STEP

		step=`expr $step + 1`
		continue;;
	*)
		break;;
    esac
done

if [ -f $INSTALL_STEP ]
then
	\rm $INSTALL_STEP
fi

if [ -f $TMP ]
then
	\rm $TMP
fi
