#!/bin/sh

##############################
# Macro Define 
##############################

TODAY=`date "+%y%m%d"`

SYSTEM=`uname`
WHOAMI=`whoami`
GZIP=gzip
KMTUNE=/usr/sbin/kmtune

ALTIBASE_HOME="$HOME/altibase_home"
ADMIN="${ALTIBASE_HOME}/bin/isql -u sys -p manager -sysdba"
ISQL="${ALTIBASE_HOME}/bin/isql -u sys -p manager"
CUR_KERNEL_FILE="${ALTIBASE_HOME}/conf/altibase.kernel.$TODAY"
REC_KERNEL_FILE="${ALTIBASE_HOME}/conf/altibase.kernel"

OPEN_FILES=2048

if [ $SYSTEM = "HP-UX" ]
then
	GZIP=`/usr/contrib/bin/gzip`
else
	GZIP=`gzip`
fi


##############################
# Function Define 
##############################

chk_os ()
{
	if [ $SYSTEM != "HP-UX" -a $SYSTEM != "AIX" -a $SYSTEM != "SunOS" -a $SYSTEM != "Linux" -a $SYSTEM != "OSF1" ]
	then
		echo "  Sorry, not support install.sh for this platform!!!" 
		echo "  You can install altibase manually." 
		exit
	fi
}

set_ker_para ()
{
    if [ $SYSTEM = "HP-UX" ]
    then
		# print into CUR_KERNEL_FILE
        echo "  Current Values" > $CUR_KERNEL_FILE
        echo "-------------------------------------" >> $CUR_KERNEL_FILE
        eval "$KMTUNE | grep shmmax" >> $CUR_KERNEL_FILE
        eval "$KMTUNE | grep shmmni" >> $CUR_KERNEL_FILE
        eval "$KMTUNE | grep shmseg" >> $CUR_KERNEL_FILE
        eval "$KMTUNE | grep semmap" >> $CUR_KERNEL_FILE
        eval "$KMTUNE | grep semmni" >> $CUR_KERNEL_FILE
        eval "$KMTUNE | grep semmns" >> $CUR_KERNEL_FILE
        eval "$KMTUNE | grep semmnu" >> $CUR_KERNEL_FILE
        eval "$KMTUNE | grep semume" >> $CUR_KERNEL_FILE
        eval "$KMTUNE | grep max_thread_proc" >> $CUR_KERNEL_FILE
        eval "$KMTUNE | grep maxusers" >> $CUR_KERNEL_FILE
        eval "$KMTUNE | grep maxdsiz" >> $CUR_KERNEL_FILE
        echo "$KMTUNE | grep maxdsiz_64bit" >> $CUR_KERNEL_FILE

        echo "#  Recommand Values" > $REC_KERNEL_FILE
        echo "#-------------------------------------" >> $REC_KERNEL_FILE
        echo "#  shmmax = more than 2147483648" >> $REC_KERNEL_FILE
        echo "#  shmmni = more than 500" >> $REC_KERNEL_FILE
        echo "#  shmseg = more than 200" >> $REC_KERNEL_FILE
        echo "#  semmap = more than semmni + 1" >> $REC_KERNEL_FILE
        echo "#  semmni = more than 1000" >> $REC_KERNEL_FILE
        echo "#  semmns = more than 4096" >> $REC_KERNEL_FILE
        echo "#  semmnu = more than semmni" >> $REC_KERNEL_FILE
        echo "#  semume = more than semmni" >> $REC_KERNEL_FILE
        echo "#  max_thread_proc = more than 600" >> $REC_KERNEL_FILE
        echo "#  maxusers = more than 64" >> $REC_KERNEL_FILE
        echo "#  maxdsiz = more than 1073741824" >> $REC_KERNEL_FILE
        echo "#  maxdsiz_64bit = more than max altibase process size" >> $REC_KERNEL_FILE
		echo >> $REC_KERNEL_FILE

        echo "#  Setting Kernel Parameters" >> $REC_KERNEL_FILE
        echo "#-------------------------------------" >> $REC_KERNEL_FILE
        echo "$KMTUNE -s shmmax=2147483648" >> $REC_KERNEL_FILE
        echo "$KMTUNE -s shmmni=500" >> $REC_KERNEL_FILE
        echo "$KMTUNE -s shmseg=200" >> $REC_KERNEL_FILE
        echo "$KMTUNE -s semmap=10001" >> $REC_KERNEL_FILE
        echo "$KMTUNE -s semmni=1000" >> $REC_KERNEL_FILE
        echo "$KMTUNE -s semmns=4096" >> $REC_KERNEL_FILE
        echo "$KMTUNE -s semmnu=1000" >> $REC_KERNEL_FILE
        echo "$KMTUNE -s semume=1000" >> $REC_KERNEL_FILE
        echo "$KMTUNE -s max_thread_proc=600" >> $REC_KERNEL_FILE
        echo "$KMTUNE -s maxusers=64" >> $REC_KERNEL_FILE
        echo "$KMTUNE -s maxdsiz=1073741824" >> $REC_KERNEL_FILE
        echo "$KMTUNE -s maxdsiz_64bit=4294967296" >> $REC_KERNEL_FILE
			
		maxdsiz_64bit=`$KMTUNE | grep maxdsiz_64bit | awk '{print $2}'`
        if [ $maxdsiz_64bit < "4294967296" ]
        then
        	echo "  !!! The maxdsiz_64bit value must be which is more than at least 4G than. !!!"
			echo
			echo "  Of the following two options : "
			echo
			echo "   1) The maxdsiz_64bit value modifies and applies and then it continues install altibase."
			echo "   *) It continues install altibase and then the maxdsiz_64bit value modifies and applies."
			echo
			echo -n "  Which would you like to perform? 1) "
			read res
        	if [ x$res = "x" -o $res = "1" ]
        	then 
				exit;
        	fi
		fi
    elif [ $SYSTEM = "AIX" ]
    then
        echo "  Current Values" > $CUR_KERNEL_FILE
        echo "-------------------------------------" >> $CUR_KERNEL_FILE
        eval "ulimit -a" >> $CUR_KERNEL_FILE

        echo "  Recommand Values (/etc/security/limits - default:)" > $REC_KERNEL_FILE
        echo "---------------------------------------------------" >> $REC_KERNEL_FILE
        echo "  fsize = -1" >> $REC_KERNEL_FILE
        echo "  data = -1" >> $REC_KERNEL_FILE
        echo "  rss = -1" >> $REC_KERNEL_FILE
    elif [ $SYSTEM = "SunOS" ]
    then
        echo "  Current Values" > $CUR_KERNEL_FILE
        echo "-------------------------------------" >> $CUR_KERNEL_FILE
        eval "sysdef -i | grep SHM" >> $CUR_KERNEL_FILE
        eval "sysdef -i | grep SEM" >> $CUR_KERNEL_FILE

        echo "  Recommand Values" > $REC_KERNEL_FILE
        echo "-------------------------------------" >> $REC_KERNEL_FILE
        echo "  shminfo_shmmax = more than 2147483648" >> $REC_KERNEL_FILE
        echo "  shminfo_shmmin = more than 1" >> $REC_KERNEL_FILE
        echo "  shminfo_shmmni = more than 500" >> $REC_KERNEL_FILE
        echo "  shminfo_shmseg = more than 200" >> $REC_KERNEL_FILE
        echo "  seminfo_semmns = more than 8192" >> $REC_KERNEL_FILE
        echo "  seminfo_semmni = more than 5029" >> $REC_KERNEL_FILE
        echo "  seminfo_semmsl = more than 2000" >> $REC_KERNEL_FILE
        echo "  seminfo_semmap = more than 5024" >> $REC_KERNEL_FILE
        echo "  seminfo_semmnu = more than 1024" >> $REC_KERNEL_FILE
        echo "  seminfo_semopm = more than 512" >> $REC_KERNEL_FILE
        echo "  seminfo_semume = more than 512" >> $REC_KERNEL_FILE
        echo "  rlim_fd_max = more than 4096" >> $REC_KERNEL_FILE
        echo "  rlim_fd_cur = more than 2048" >> $REC_KERNEL_FILE
    elif [ $SYSTEM = "Linux" ]
    then
        echo "  Will be append following into /etc/rc.d/rc.local" > $REC_KERNEL_FILE
        echo "-------------------------------------------------" >> $REC_KERNEL_FILE
        echo "  echo 2147483648 > /proc/sys/kernel/shmmax" >> $REC_KERNEL_FILE
        echo "  echo 512 32000 512 512 > /proc/sys/kernel/sem" >> $REC_KERNEL_FILE
    elif [ $SYSTEM = "OSF1" ]
    then
            echo "  Current Values" >> $CUR_KERNEL_FILE
            echo "-------------------------------------" >> $CUR_KERNEL_FILE
            /sbin/sysconfig -q ipc | grep shm >> $CUR_KERNEL_FILE
            /sbin/sysconfig -q ipc | grep sem >> $CUR_KERNEL_FILE
            /sbin/sysconfig -q proc | grep per_proc_address >> $CUR_KERNEL_FILE
            /sbin/sysconfig -q proc | grep per_proc_data >> $CUR_KERNEL_FILE
            /sbin/sysconfig -q proc | grep thread >> $CUR_KERNEL_FILE
            /sbin/sysconfig -q vm | grep mapentries >> $CUR_KERNEL_FILE
            /sbin/sysconfig -q vm | grep vpagemax >> $CUR_KERNEL_FILE
            echo

            echo "  Recommand Values" > $REC_KERNEL_FILE
            echo "-------------------------------------" >> $REC_KERNEL_FILE
            echo "  shm-max = 2147483648" >> $REC_KERNEL_FILE
            echo "  shm_min = 1" >> $REC_KERNEL_FILE
            echo "  shm_mni = 500" >> $REC_KERNEL_FILE
            echo "  shm_seg = 512" >> $REC_KERNEL_FILE
            echo "  sem-mni = 1024" >> $REC_KERNEL_FILE
            echo "  sem-msl = 512" >> $REC_KERNEL_FILE
            echo "  sem-opm = 512" >> $REC_KERNEL_FILE
            echo "  sem-ume = 512" >> $REC_KERNEL_FILE
            echo "  sem-vmx = 70000" >> $REC_KERNEL_FILE
            echo "  sem-aem = 16384" >> $REC_KERNEL_FILE
            echo "  max-threads-per-user = 1024" >> $REC_KERNEL_FILE
            echo "  thread-max = 2048" >> $REC_KERNEL_FILE
            echo "  mapentries = 400" >> $REC_KERNEL_FILE

            /usr/sbin/uerf -r 300 | grep "physical memory" > tmp.txt &
            kill -9 $!
            mem_size=`head -1 tmp.txt | awk '{print $4}'`
            vpagemax=`expr ($mem_size \* 1024 \* 1024) / 8192`
            echo "  vm-vpagemax = $vpagemax" >> $REC_KERNEL_FILE

            echo -n "  Please specify max_per_proc_data_size value (default 4294967296) : "
            read res
            if [ x$res = "x" ]
            then
                    res=4294967296
            fi
            echo "  per-proc-address-space = $res" >> $REC_KERNEL_FILE
            echo "  max-per-proc-address-space = $res" >> $REC_KERNEL_FILE
            echo "  max-per-proc-data-size = $res" >> $REC_KERNEL_FILE
            echo "  per-proc-data-size = $res" >> $REC_KERNEL_FILE
    fi

    echo
    echo "  You have to reboot system after install altibase, if you want to apply above values."
}

set_env ()
{
	echo -n "  Would you like to install altibase into $ALTIBASE_HOME? (yes) "
	read res
	if [ x$res != "x" ]
	then
		if [ $res != "yes" -a $res != "y" -a $res != "YES" -a $res != "Y" ]
		then
			echo -n "  Please input the directory for install altibase : "
			read res
			if [ x$res = "x" ]
			then
				echo "  You don't specify the directory for install altibase."
				echo "  Please try perform install.sh again."
				exit
			else
				ALTIBASE_HOME=$res
			fi
		fi
	fi
		
	echo
	eval "ulimit -a"
	echo
	echo -n "  Please specify file descriptor limit (1024) : "
	read open_files
	if [ x$open_files = "x" ]
	then
		open_files=1024
	fi

	echo
	echo "  ALTIBASE_HOME=$ALTIBASE_HOME" 
	echo "  PATH=\$ALTIBASE_HOME/bin:\$PATH" 
	echo "  LD_LIBRARY_PATH=\$ALTIBASE_HOME/lib:\$LD_LIBRARY_PATH" 
	echo "  CLASSPATH=\$ALTIBASE_HOME/lib/Altibase.jar:\$CLASSPATH" 

	if [ $SYSTEM = "SunOS" ]
	then
		echo "  LD_LIBRARY_PATH_64=\$ALTIBASE_HOME/lib:\$LD_LIBRARY_PATH_64" 
	elif [ $SYSTEM = "HP-UX" ]
	then
		echo "  SHLIB_PATH=\$ALTIBASE_HOME/lib:\$SHLIB_PATH" 
	fi

	echo "  ulimit -d unlimited"
	echo "  ulimit -f unlimited"
	echo "  ulimit -n $open_files"
	echo
	
	cd $HOME
	mysh=`env | grep SHELL | tr -d "SHELL="`
	mysh=`basename $mysh`

	case $mysh in
	csh)
		echo -n "  Please input shell init file name (.cshrc) : "
		read profile
		if [ x$profile = "x" ]
		then
	    	profile=".cshrc"
		fi;;
	bash)
		echo -n "  Please input shell init file name (.bash_profile) : "
		read profile
		if [ x$profile = "x" ]
		then
	    	profile=".bash_profile"
		fi;;
	ksh)
		echo -n "  Please input shell init file name (.kshrc) : "
		read profile
		if [ x$profile = "x" ]
		then
	    	profile=".kshrc"
		fi;;
	sh)
		echo -n "  Please input shell init file name (.profile) : "
		read profile
		if [ x$profile = "x" ]
		then
	    	profile=".profile"
		fi;;
	*)
		echo "  Sorry, you have to set above enveroment variables in your shell init file yourself." 
		return;;
	esac

	echo "" >> $profile
	echo "# For Altibase" >> $profile

	case $mysh in
	csh)
		echo "setenv ALTIBASE_HOME $ALTIBASE_HOME" >> $profile
		echo "setenv PATH \$ALTIBASE_HOME/bin:\$PATH" >> $profile
		echo "setenv LD_LIBRARY_PATH \$ALTIBASE_HOME/lib:\$LD_LIBRARY_PATH" >> $profile
		echo "setenv CLASSPATH \$ALTIBASE_HOME/lib/Altibase.jar:\$CLASSPATH" >> $profile
		if [ $SYSTEM = "SunOS" ]
		then
			echo "setenv LD_LIBRARY_PATH_64 \$ALTIBASE_HOME/lib:\$LD_LIBRARY_PATH_64" >> $profile 
		elif [ $SYSTEM = "HP-UX" ]
		then
			echo "setenv SHLIB_PATH \$ALTIBASE_HOME/lib:\$SHLIB_PATH" >> $profile 
			echo >> $profile
			echo "setenv PTHREAD_DISABLE_HANDOFF ON >> $profile
			echo "setenv PTHREAD_FORCE_SCOPE_SYSTEM 1 >> $profile
			echo "setenv PERF_ENABLE 1 >> $profile
			echo "setenv _M_ARENA_OPTS 20:32 >> $profile
		elif [ $SYSTEM = "AIX" ]
		then
			echo "setenv AIXTHREAD_MUTEX_DEBUG OFF" >> $profile
			echo "setenv AIXTHREAD_RWLOCK_DEBUG OFF" >> $profile
			echo "setenv AIXTHREAD_COND_DEBUG OFF" >> $profile
			echo "setenv AIXTHREAD_MNRATIO 1:1" >> $profile
			echo "setenv AIXTHREAD_SCOPE S" >> $profile
			echo "setenv MALLOCMULTIHEAP 1" >> $profile
		fi;;
	*)
		echo "export ALTIBASE_HOME=$ALTIBASE_HOME" >> $profile
		echo "export PATH=\$ALTIBASE_HOME/bin:\$PATH" >> $profile
		echo "export LD_LIBRARY_PATH=\$ALTIBASE_HOME/lib:\$LD_LIBRARY_PATH" >> $profile
		echo "export CLASSPATH=\$ALTIBASE_HOME/lib/Altibase.jar:\$CLASSPATH" >> $profile
		if [ $SYSTEM = "SunOS" ]
		then
			echo "export LD_LIBRARY_PATH_64=\$ALTIBASE_HOME/lib:\$LD_LIBRARY_PATH_64" >> $profile 
		elif [ $SYSTEM = "HP-UX" ]
		then
			echo "export SHLIB_PATH=\$ALTIBASE_HOME/lib:\$SHLIB_PATH" >> $profile 
			echo >> $profile
			echo "export PTHREAD_DISABLE_HANDOFF=ON >> $profile
			echo "export PTHREAD_FORCE_SCOPE_SYSTEM=1 >> $profile
			echo "export PERF_ENABLE=1 >> $profile
			echo "export _M_ARENA_OPTS=20:32 >> $profile
		elif [ $SYSTEM = "AIX" ]
		then
			echo "export AIXTHREAD_MUTEX_DEBUG=OFF" >> $profile
			echo "export AIXTHREAD_RWLOCK_DEBUG=OFF" >> $profile
			echo "export AIXTHREAD_COND_DEBUG=OFF" >> $profile
			echo "export AIXTHREAD_MNRATIO=1:1" >> $profile
			echo "export AIXTHREAD_SCOPE=S" >> $profile
			echo "export MALLOCMULTIHEAP=1" >> $profile
		fi;;
	esac

	echo "" >> $profile
	echo "ulimit -d unlimited" >> $profile
	echo "ulimit -f unlimited" >> $profile
	echo "ulimit -n $open_files" >> $profile
}

set_license ()
{
	echo -n "  Please input the license key : "
	read license
	echo $license >> $ALTIBASE_HOME/conf/license
	
	#if [ -f $ALTIBASE_HOME/conf/license ]
	#then
	#	echo $license >> $ALTIBASE_HOME/conf/license
	#else
	#	echo $license > $ALTIBASE_HOME/conf/license
	#fi
}

get_property ()
{
	echo
	echo -n "  Memory Database File Directory0 ($ALTIBASE_HOME/dbs) : "
	read MEM_DB_DIR0
	if [ x$MEM_DB_DIR0 = "x" ]
	then
	    MEM_DB_DIR0="?/dbs"
	fi
	
	echo -n "  Memory Database File Directory1 ($ALTIBASE_HOME/dbs) : "
	read MEM_DB_DIR1
	if [ x$MEM_DB_DIR1 = "x" ]
	then
	    MEM_DB_DIR1="?/dbs"
	fi
	
	echo -n "  Disk Database File Directory ($ALTIBASE_HOME/dbs) : "
	read DEFAULT_DISK_DB_DIR
	if [ x$DEFAULT_DISK_DB_DIR = "x" ]
	then
	    DEFAULT_DISK_DB_DIR="?/dbs"
	fi
	
	echo -n "  Log File Directory ($ALTIBASE_HOME/logs) : "
	read LOG_DIR
	if [ x$LOG_DIR = "x" ]
	then
	    LOG_DIR="?/logs"
	fi
	
	echo -n "  Archive Log File Directory ($ALTIBASE_HOME/arch_logs) : "
	read ARCHIVE_DIR
	if [ x$ARCHIVE_DIR = "x" ]
	then
	    ARCHIVE_DIR="?/arch_logs"
	fi
	
	echo -n "  Shared Memory DB Key (0) : "
	read SHM_DB_KEY
	if [ x$SHM_DB_KEY = "x" ]
	then
	    SHM_DB_KEY=0
	fi
	
	echo -n "  Max Memory DB Size (4G) : "
	read MEM_MAX_DB_SIZE
	if [ x$MEM_MAX_DB_SIZE = "x" ]
	then
	    MEM_MAX_DB_SIZE=4G
	fi
	
	echo -n "  Buffer Pool Size(page count) for Disk Tables (16384) : "
	read BUFFER_POOL_SIZE
	if [ x$BUFFER_POOL_SIZE = "x" ]
	then
	    BUFFER_POOL_SIZE=16384
	fi
	
	echo -n "  Max IPC Connection Count (0) : "
	read IPC_CHANNEL_COUNT
	if [ x$IPC_CHANNEL_COUNT = "x" ]
	then
	    IPC_CHANNEL_COUNT=0
	fi
	
	echo -n "  NLS_USE (US7ASCII) : "
	read NLS_USE
	if [ x$NLS_USE = "x" ]
	then
	    NLS_USE=US7ASCII
	fi
	
	echo -n "  Max Client (1000) : "
	read MAX_CLIENT
	if [ x$MAX_CLIENT = "x" ]
	then
	    MAX_CLIENT=1000
	fi
	
	echo -n "  Replication Port No (0) : "
	read REPLICATION_PORT_NO
	if [ x$REPLICATION_PORT_NO = "x" ]
	then
	    REPLICATION_PORT_NO=0
	fi
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
			if [ ${prop_name:0:1} = "#" ]
			then
				echo $val >> prop.tmp
				continue;
			fi
		
			case $prop_name in
			MEM_DB_DIR)
				if [ $i = 0 ]
				then
					echo "MEM_DB_DIR = $MEM_DB_DIR0" >> prop.tmp;
					i=1;
				else
					echo "MEM_DB_DIR = $MEM_DB_DIR1" >> prop.tmp;
				fi;;
			DEFAULT_DISK_DB_DIR)
				echo "DEFAULT_DISK_DB_DIR = $DEFAULT_DISK_DB_DIR" >> prop.tmp;;
			LOG_DIR)
				echo "LOG_DIR = $LOG_DIR" >> prop.tmp;;
			ARCHIVE_DIR)
				echo "ARCHIVE_DIR = $ARCHIVE_DIR" >> prop.tmp;;
			SHM_DB_KEY)
				echo "SHM_DB_KEY = $SHM_DB_KEY" >> prop.tmp;;
			MEM_MAX_DB_SIZE)
				echo "MEM_MAX_DB_SIZE = $MEM_MAX_DB_SIZE" >> prop.tmp;;
			BUFFER_POOL_SIZE)
				echo "BUFFER_POOL_SIZE = $BUFFER_POOL_SIZE" >> prop.tmp;;
			IPC_CHANNEL_COUNT)
				echo "IPC_CHANNEL_COUNT = $IPC_CHANNEL_COUNT" >> prop.tmp;;
			NLS_USE)
				echo "NLS_USE = $NLS_USE" >> prop.tmp;;
			MAX_CLIENT)
				echo "MAX_CLIENT = $MAX_CLIENT" >> prop.tmp;;
			REPLICATION_PORT_NO)
				echo "REPLICATION_PORT_NO = $REPLICATION_PORT_NO" >> prop.tmp;;
			*)
				echo $val >> prop.tmp;;
			esac
		fi
	done

	mv altibase.properties altibase.properties.org
	mv prop.tmp altibase.properties
}

crt_db ()
{
	echo
    echo -n "Database Name (mydb) : "
    read dbname
    if [ x$dbname = "x" ]
    then
    	dbname=mydb
    fi
    
    echo -n "Memory DB InitSize (10M) : "
    read initsize
    if [ x$initsize = "x" ]
    then
    	initsize=10M
    fi
    
    echo -n "MEM DB FileSize (2G) : "
    read filesize
    if [ x$filesize = "x" ]
    then
    	filesize=2G
    fi
    
    echo -n "Archive Log Mode (noarchivelog) : "
    read arch
    if [ x$arch = "x" ]
    then
    	arch=noarchivelog
    fi
    
    ${ADMIN} <<EOF
    startup process;
    create database $dbname INITSIZE=$initsize, filesize=$filesize $arch;
    shutdown abort
    quit
EOF
}


##############################
# install2.sh start
##############################

echo
echo "Altibase Installation Start!!"
echo

chk_os

echo -n "  Altibase user is $WHOAMI. Is right? (yes) "
read res
if [ x$res != "x" ] 
then
	if [ $res != "Y" -o $res != "y" -o $res != "YES" -o $res != "yes" ]
	then
		echo "  Please try install2.sh after login altibase user."
    	exit;
	fi
fi

echo "  Of the following two options : "
echo
echo "   1) New install"
echo "   *) Patch or Upgrade"
echo
echo -n "  Which would you like to perform? 1) "
read install_option

if [ x$install_option = "x" ] || [ $install_option = 1 ]
then
	echo
	echo "<<< Enviroment Variables Setting >>>"
	echo
	
	set_env

	echo
	echo "<<< License Setting >>>"
	echo

	set_license
fi		

echo
echo "<<< Altibase Property Setting >>>"
echo

echo "  Of the following three options : "
echo
echo "   1) Use default altibase property file"
echo "   *) Edit altibase property file"
#echo "   *) Use current altibase property file"
echo
echo -n "  Which would you like to perform? 1) "
read res
if [ x$res != "x" ] && [ $res = 2 ]
then
	get_property
	set_property
fi

echo
echo "<<< Database Creation >>>"
echo

echo "  Of the following two options : "
echo
echo "   1) Create new database"
echo "   *) Use current database"
echo
echo -n "  Which would you like to perform? 1) "
read res
if [ x$res = "x" ] || [ $res = 1 ]
then
	crt_db
fi

echo
echo "<<< Startup Altibase >>>"
echo

echo -n "Would you like to altibase startup? (yes) "
read res
if [ x$res = "x" ] 
then
	eval "server start"
elif [ $res = "Y" -o $res = "y" -o $res = "YES" -o $res = "yes" ]
then
	eval "server start"
fi
