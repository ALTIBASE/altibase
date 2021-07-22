#!/bin/sh

##############################
# Macro Define 
##############################

SYSTEM=`uname`
WHOAMI=`whoami`

x=0
if [ $x = 1 ]
then
if [ $SYSTEM = "HP-UX" ]
then
    GROUPADD = `groupadd`
    USERADD  = `useradd`
elif [ $SYSTEM = "AIX" ]
then
    GROUPADD = `mkgroup`
    USERADD  = `mkuser`
elif [ $SYSTEM = "SunOS" ]
then
    GROUPADD = `groupadd`
    USERADD  = `useradd`
elif [ $SYSTEM = "Linux" ]
then
    GROUPADD = `groupadd`
    USERADD  = `useradd`
elif [ $SYSTEM = "OSF1" ]
then
    GROUPADD = `groupadd`
    USERADD  = `useradd`
fi
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
	# Root have to perform it.
	# You can choose the parameter for setting into new value.

	if [ $WHOAMI != "root" ]
	then
		echo "  Root user can only set kernel parameter."
		echo
		echo "  Of the following two options : "
		echo
		echo "   1) Quit and login root and then try install.sh again"
		echo "   *) Skip setting kernel parameter"
		echo
		echo -n "  Which would you like to perform? 1) "
		read res
		if [ x$res = "x" ] || [ $res = 1 ] 
		then
			exit;
		else
			return;
		fi
	fi

	if [ $SYSTEM = "HP-UX" ]
	then
		echo "  Current Values"
		echo "-------------------------------------"
		eval "kmtune | grep shmmax"
		eval "kmtune | grep shmmni"
		eval "kmtune | grep shmseg"
		eval "kmtune | grep semmap"
		eval "kmtune | grep semmni"
		eval "kmtune | grep semmns"
		eval "kmtune | grep semmnu"
		eval "kmtune | grep semume"
		eval "kmtune | grep max_thread_proc"
		eval "kmtune | grep maxusers"
		eval "kmtune | grep maxdsiz"
		echo

		echo "  New Values"
		echo "-------------------------------------"
		echo "  shmmax=2147483648"
		echo "  shmmni=500"
		echo "  shmseg=200"
		echo "  semmap=10001"
		echo "  semmni=1000"
		echo "  semmns=4096"
		echo "  semmnu=1000"
		echo "  semume=1000"
		echo "  max_thread_proc=600"
		echo "  maxusers=64"
		echo "  maxdsiz=1073741824"

		echo -n "  Please specify maxdsiz_64bit value (default 4294967296) : "
		read res
		if [ x$res = "x" ]
		then
			res=4294967296
		fi

		echo "  maxdsiz_64bit=$res"
		echo
		echo -n "  Are you agree to change into new values? (yes) "
		read res
		if [ x$res = "x" -o $res = "y" -o $res = "yes" -o $res = "Y" -o $res = "YES" ]
		then 
			kmtune -s maxdsiz_64bit=$res
			kmtune -s shmmax=2147483648
			kmtune -s shmmni=500
			kmtune -s shmseg=200
			kmtune -s semmap=10001
			kmtune -s semmni=1000
			kmtune -s semmns=4096
			kmtune -s semmnu=1000
			kmtune -s semume=1000
			kmtune -s max_thread_proc=600
			kmtune -s maxusers=64
			kmtune -s maxdsiz=1073741824
		fi
	elif [ $SYSTEM = "AIX" ]
	then
		echo "  New Values (/etc/security/limits - default:)"
		echo "---------------------------------------------------"
		echo "  fsize = -1"
		echo "  data = -1"
		echo "  rss = -1"

		echo
		echo -n "  Are you agree to change into new values? (yes) "
		read res
		if [ x$res = "x" -o $res = "y" -o $res = "yes" -o $res = "Y" -o $res = "YES" ]
		then 
			vi /etc/security/limits << END
			:%s/fsize[ =0-9]*/fsize = -1/g
			:%s/data[ =0-9]*/data = -1/g
			:%s/rss[ =0-9]*/rss = -1/g
			:wq!
END
		fi
	elif [ $SYSTEM = "SunOS" ]
	then
		echo "  Current Values"
		echo "-------------------------------------"
		eval "sysdef -i | grep SHM"
		eval "sysdef -i | grep SEM"
		echo

		echo "  New Values"
		echo "-------------------------------------"
		echo "  shminfo_shmmax = 2147483648"
		echo "  shminfo_shmmin = 1"
		echo "  shminfo_shmmni = 500"
		echo "  shminfo_shmseg = 200"
		echo "  seminfo_semmns = 8192"
		echo "  seminfo_semmni = 5029"
		echo "  seminfo_semmsl = 2000"
		echo "  seminfo_semmap = 5024"
		echo "  seminfo_semmnu = 1024"
		echo "  seminfo_semopm = 512"
		echo "  seminfo_semume = 512"
		echo "  priority_paging = 1 (SunOS 5.7)"

		echo
		echo -n "  Are you agree to change into new values? (yes) "
		read res
		if [ x$res = "x" ] || [ $res = "y" -o $res = "yes" -o $res = "Y" -o $res = "YES" ]
		then 
			vi /etc/sysconfigtab << END
			:%s/set[ ]*shmsys:shminfo_shmmax/#set shmsys:shminfo_shmmax/g
			:%s/set[ ]*shmsys:shminfo_shmmin/#set shmsys:shminfo_shmmin/g
			:%s/set[ ]*shmsys:shminfo_shmmni/#set shmsys:shminfo_shmmni/g
			:%s/set[ ]*shmsys:shminfo_shmseg/#set shmsys:shminfo_shmseg/g
			:%s/set[ ]*semsys:seminfo_semmns/#set semsys:seminfo_semmns/g
			:%s/set[ ]*semsys:seminfo_semmni/#set semsys:seminfo_semmni/g
			:%s/set[ ]*semsys:seminfo_semmsl/#set semsys:seminfo_semmsl/g
			:%s/set[ ]*semsys:seminfo_semmap/#set semsys:seminfo_semmap/g
			:%s/set[ ]*semsys:seminfo_semmnu/#set semsys:seminfo_semmnu/g
			:%s/set[ ]*semsys:seminfo_semopm/#set semsys:seminfo_semopm/g
			:%s/set[ ]*semsys:seminfo_semume/#set semsys:seminfo_semume/g
			:%s/set[ ]*priority_paging/#set priority_paging/g
			:wq!
END
			echo >> /etc/system
			echo "set shmsys:shminfo_shmmax = 2147483648" >> /etc/system
			echo "set shmsys:shminfo_shmmin = 1" >> /etc/system
			echo "set shmsys:shminfo_shmmni = 500" >> /etc/system
			echo "set shmsys:shminfo_shmseg = 200" >> /etc/system
			echo "set semsys:seminfo_semmns = 8192" >> /etc/system
			echo "set semsys:seminfo_semmni = 5029" >> /etc/system
			echo "set semsys:seminfo_semmsl = 2000" >> /etc/system
			echo "set semsys:seminfo_semmap = 5024" >> /etc/system
			echo "set semsys:seminfo_semmnu = 1024" >> /etc/system
			echo "set semsys:seminfo_semopm = 512" >> /etc/system
			echo "set semsys:seminfo_semume = 512" >> /etc/system

			os_ver=`uname -r`
			if [ $os_ver = "5.7" ]
			then
				echo "set priority_paging = 1" >> /etc/system
			fi
		fi
	elif [ $SYSTEM = "Linux" ]
	then
		echo "  Will be append following into /etc/rc.d/rc.local"
		echo "-------------------------------------------------"
		echo "  echo 2147483648 > /proc/sys/kernel/shmmax"
		echo "  echo 512 32000 512 512 > /proc/sys/kernel/sem"

		echo
		echo -n "  Are you agree to change into new values? (yes) "
		read res
		if [ x$res = "x" -o $res = "y" -o $res = "yes" -o $res = "Y" -o $res = "YES" ]
		then 
			echo >> /etc/rc.d/rc.local
			echo "echo 2147483648 > /proc/sys/kernel/shmmax" >> /etc/rc.d/rc.local
			echo "echo 512 32000 512 512 > /proc/sys/kernel/sem" >> /etc/rc.d/rc.local
		fi
	elif [ $SYSTEM = "OSF1" ]
	then
		echo "  Current Values"
		echo "-------------------------------------"
		/sbin/sysconfig -q ipc | grep shm
		/sbin/sysconfig -q ipc | grep sem
		/sbin/sysconfig -q proc | grep per_proc_address
		/sbin/sysconfig -q proc | grep per_proc_data
		/sbin/sysconfig -q proc | grep thread
		/sbin/sysconfig -q vm | grep mapentries
		/sbin/sysconfig -q vm | grep vpagemax
		echo

		echo "  New Values"
		echo "-------------------------------------"
		echo "  shm-max = 2147483648"
		echo "  shm_min = 1"
		echo "  shm_mni = 500"
		echo "  shm_seg = 512"
		echo "  sem-mni = 1024"
		echo "  sem-msl = 512"
		echo "  sem-opm = 512"
		echo "  sem-ume = 512"
		echo "  sem-vmx = 70000"
		echo "  sem-aem = 16384"
		echo "  max-threads-per-user = 1024"
		echo "  thread-max = 2048"
		echo "  mapentries = 400"

		/usr/sbin/uerf -r 300 | grep "physical memory" > tmp.txt &
		kill -9 $!
		mem_size=`head -1 tmp.txt | awk '{print $4}'`
		vpagemax=`expr ($mem_size \* 1024 \* 1024) / 8192`
		echo "  vm-vpagemax = $vpagemax"

		echo -n "  Please specify max_per_proc_data_size value (default 4294967296) : "
		read res
		if [ x$res = "x" ]
		then
			res=4294967296
		fi
		echo "  per-proc-address-space = $res"
		echo "  max-per-proc-address-space = $res"
		echo "  max-per-proc-data-size = $res"
		echo "  per-proc-data-size = $res"

		echo
		echo -n "  Are you agree to change into new values? (yes) "
		read res
		if [ x$res = "x" -o $res = "y" -o $res = "yes" -o $res = "Y" -o $res = "YES" ]
		then 
			# how to case of not found parameter?
			vi /etc/sysconfigtab << END
			:%s/shm-max[ =0-9]*/shm-max = 2147483648/g
			:%s/shm-min[ =0-9]*/shm-min = 1/g
			:%s/shm-mni[ =0-9]*/shm-mni = 500/g
			:%s/shm-seg[ =0-9]*/shm-seg = 512/g
			:%s/sem-mni[ =0-9]*/sem-mni = 1024/g
			:%s/sem-msl[ =0-9]*/sem-msl = 512/g
			:%s/sem-opm[ =0-9]*/sem-opm = 512/g
			:%s/sem-ume[ =0-9]*/sem-ume = 512/g
			:%s/sem-vmx[ =0-9]*/sem-vmx = 70000/g
			:%s/sem-aem[ =0-9]*/sem-aem = 16384/g
			:%s/max-threads-per-user[ =0-9]*/max-threads-per-user = 1024/g
			:%s/thread-max[ =0-9]*/thread-max = 2048/g
			:%s/mapentries[ =0-9]*/mapentries = 400/g
			:%s/vm-vpagemax[ =0-9]*/vm-vpagemax = $vpagemax/g
			:%s/per-proc-address-space[ =0-9]*/per-proc-address-space = $res/g
			:%s/max-per-proc-address-space[ =0-9]*/max-per-proc-address-space = $res/g
			:%s/max-per-proc-data-size[ =0-9]*/max-per-proc-data-size = $res/g
			:%s/per-proc-data-size[ =0-9]*/per-proc-data-size = $res/g
			:wq!
END
		fi
	fi

	echo
	echo "  You have to reboot system after install altibase, if you want to apply above values."
}


##############################
# install1.sh start
##############################

echo
echo "Altibase Pre-Installation Start!!"
echo

chk_os

if [ $WHOAMI != "root" ]
then
	echo "  Root user can only perform install1.sh."
	echo "  Please try install1.sh after login root."
	exit;
fi

#echo
#echo "<<< Group Creation >>>"
#echo
	
#echo
#echo "<<< User Creation >>>"
#echo
	
echo
echo "<<< Kernel Parameter Setting >>>"
echo
	
set_ker_para

echo "  Please execute install2.sh after login altibase user."
