#!/bin/sh
SYSTEM=`uname`

KMTUNE=/usr/sbin/kmtune
KCTUNE=/usr/sbin/kctune

#echo "test code $SYSTEM"
#echo "test code $CHECK_SHELL"

chk_ker_para ()
{
    if [ $SYSTEM = "HP-UX" -o $SYSTEM = "HP-RX" ]
	then
        echo "  [ Recommended Kernel Parametere Values ] " 
        echo "  shmmax = 2147483648" 
        echo "  shmmni = 500" 
        echo "  shmseg = 200" 
        echo "  semmap = 1001" 
        echo "  semmni = 1000" 
        echo "  semmns = 4096" 
        echo "  semmnu = 1000" 
        echo "  semume = 1000" 
        echo "  max_thread_proc = 600" 
        echo "  maxusers = 64" 
		echo "  dbc_min_pct = 5"
		echo "  dbc_max_pct = 5"
        echo "  maxdsiz = 1073741824" 
        echo "  maxdsiz_64bit = 4294967296" 
	    echo 
	    echo "  # maxdsiz_64bit"
        echo "  The Altibase server needs sufficient memory space in order to function properly."
        echo "  Please set the value to 70% of system memory or greater."
	    echo 
	    echo "  # dbc_min_pct and dbc_max_pct"
	    echo "  dbc_min_pct and dbc_max_pct were renamed filecache_min "
		echo "  and filecache_max, respectively, in HP version 11.31 and subsequent releases."
		echo 
        echo "  [ How to modify kernel parameter values ] " 
        echo "  $KMTUNE -s shmmax=2147483648" 
        echo "  $KMTUNE -s shmmni=500" 
        echo "  $KMTUNE -s shmseg=200" 
        echo "  $KMTUNE -s semmap=1001" 
        echo "  $KMTUNE -s semmni=1000" 
        echo "  $KMTUNE -s semmns=4096" 
        echo "  $KMTUNE -s semmnu=1000" 
        echo "  $KMTUNE -s semume=1000" 
		echo "  $KMTUNE -s max_thread_proc=600" 
		echo "  $KMTUNE -s maxusers=64" 
		echo "  $KMTUNE -s dbc_min_pct=5" 
		echo "  $KMTUNE -s dbc_max_pct=5" 
		echo "  $KMTUNE -s maxdsiz=1073741824" 
		echo "  $KMTUNE -s maxdsiz_64bit=4294967296" 
		echo
    elif [ $SYSTEM = "AIX" ]
	then
		echo "  [ Recommended Kernel Parametere Values ] " 
 	    echo "  fsize = -1"
	    echo "  data  = -1"
	    echo "  rss   = -1"
		echo
	    echo "  Maximum number of PROCESSES allowed per user"
	    echo "    = greater than the value set using the Altibase property MAX_CLIENT"
	    echo "  Size of the File System Buffer Cache"
	    echo "    = less than 20% of total memory."
	    echo "  AIO = Available"
	    echo "  AMO = AIX version 5.2 ML04 and above"
	    echo "        lru_file_repage=0"
	    echo "        strict_maxclient%=0"
	    echo "      = Versions prior to AIX version 5.2 ML04"
	    echo "        lru_file_repage=0"
		echo 
        echo "  [ How to modify kernel parameter values ]"
	    echo
		echo "  Edit /etc/security/limits file"
		echo "      Set fsize, data and rss variables."
		echo
		echo "  Run smit"
		echo "    1) System Environments" 
		echo "    System Environments > Change / Show Characteristics Of Operating System"
		echo "    Change : "
		echo "       Maximum number of PROCESSES allowed per user"
		echo "       = greater than the value set using the Altibase property MAX_CLIENT"
		echo "       Size of the File System BufferCache"
		echo "       = less than 20% of total memory."
		echo
		echo "    2) AIO "
		echo "    Device > Asynchronous I/O > Posix Asynchronous I/O > Configure Defined Asynchronous I/O"
		echo "    Change : posix_aio0=Available"
		echo
		echo "    3) AMO" 
		echo "    Performance & Resource Scheduling > "
		echo "    Tuning Kernel & Network Parameters > "
		echo "    Tuning Virtual Memory Manager, File System and Logical Volume Manager Params >" 
		echo "    + List All Characteristics of Current Parameters --- Check"
		echo "    + Change / Show Current Parameters --- Current Change"
		echo "    + Change / Show Parameters for Next Boot --- Change upon Reboot"
		echo
		echo "    When using AIX version 5.2 ML04 or above :"
		echo "        Change : lru_file_repage=0"
		echo "                 strict_maxclient%=0"
		echo "    The case of Versions prior to AIX version 5.2 ML04 :"
		echo "        Change : lru_file_repage=0"
		echo
	elif [ $SYSTEM = "SunOS" ]
    then
		Sol10=`uname -r`

	    echo "  The priority_paging parameter, which is related to" 
	    echo "  the Sun system's filesystem cache, must absolutely "
	    echo "  be set in order to prevent problems related to excessive memory "
	    echo "  use when using the filesystem cache rather than direct I/O."
	    echo "  (Applies only to SunOS version 5.7 and above.)"
	    echo
        echo "  [ Recommended Kernel Parameter Values ] " 
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
		echo "  rlim_fd_max = 4096" 
		echo "  rlim_fd_cur = 2048" 
		echo 
		echo "  [ How to modify kernel parameter values ] "

		if [ $Sol10 = "5.10" ]
        then
            echo "  == Solaris 10 ==" 
            echo "  cf)projadd -U i[user name] -K \"project.max-sem-ids=(priv,5029,deny)\" user.[user name]" 
            echo "  cf)projmod -a -K \"project.max-shm-memory=(priv, real memory max value,deny)\" user.[user name]" 
            echo  
	        echo "  # Default Values for sem- and shm-related Parameters" 
			echo "  projadd -U altibase -K \"project.max-sem-ids=(priv,5029,deny)\" user.altibase" 
	        echo "  projmod -a -K \"project.max-shm-memory=(priv, real memory max value,deny)\" user.altibase" 
            echo "  projmod -a -K \"process.max-sem-nsems=(priv,2000,deny)\" user.altibase" 
	        echo "  projmod -a -K \"process.max-sem-ops=(priv,512,deny)\" user.altibase" 
	        echo "  projmod -a -K \"project.max-shm-ids=(priv,1024,deny)\" user.altibase"
			echo
			echo "  # Message Queue-related Parameters"
	        echo "  projmod -a -K \"project.max-msg-messages=(priv,100,deny)\" user.altibase" 
	        echo "  projmod -a -K \"project.max-msg-ids=(priv,100,deny)\" user.altibase" 
	        echo "  projmod -a -K \"process.max-msg-qbytes=(priv,1048576,deny)\" user.altibase" 
			echo
        else
	        echo "  Edit /etc/system file."
            echo
			echo "  # Default Values for sem- and shm-related Parameters" 
	        echo "  set shmsys:shminfo_shmmax = 2147483648" 
	        echo "  set shmsys:shminfo_shmmin = 1" 
	        echo "  set shmsys:shminfo_shmmni = 500" 
	        echo "  set shmsys:shminfo_shmseg = 200" 
	        echo "  set semsys:seminfo_semmns = 8192" 
	        echo "  set semsys:seminfo_semmni = 5029" 
	        echo "  set semsys:seminfo_semmsl = 2000" 
	        echo "  set semsys:seminfo_semmap = 5024" 
	        echo "  set semsys:seminfo_semmnu = 1024"
	        echo "  set semsys:seminfo_semopm = 512" 
	        echo "  set semsys:seminfo_semume = 512" 
	        echo "  set rlim_fd_max = 4096" 
	        echo "  set rlim_fd_cur = 2048" 

            echo "# Message Queue-related Parameters"
            echo "projmod -a -K \"project.max-msg-messages=(priv,100,deny)\" user.altibase "
            echo "projmod -a -K \"project.max-msg-ids=(priv,100,deny)\" user.altibase"
            echo "projmod -a -K \"process.max-msg-qbytes=(priv,1048576,deny)\" user.altibase"
        fi
		echo
    elif [ $SYSTEM = "Linux" ]
    then
        echo "  [ How to modify kernel parameter values ] " 
        echo "  echo 512 32000 512 512 > /proc/sys/kernel/sem" 
        echo "  echo 872415232 > /proc/sys/kernel/shmall"
        echo 
        echo "  # shmall "
        echo "  The value of the \"shmall\" kernel parameter must be set "
        echo "  if Altibase is to be used in shared memory mode. "
        echo "  The value of this parameter, which determines the maximum "
        echo "  database size (in bytes) that Altibase can use"
		echo
	fi

	echo "
  These values must be set in order for the Altibase DBMS to run properly.
  They must be set such that they are suitable for the system configuration.
         "
    echo
    echo "  !!! You should modify kernel parameters and reboot the system to apply new kernel parameters. !!!"
}
		
##############################
# check_kernel_para.sh start
##############################


chk_ker_para


