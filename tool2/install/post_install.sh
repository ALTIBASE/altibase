#!/bin/sh

# $1 : [setenv / dbcreate] : working type
# $2 : [client]            :                 --> Client only
# $3 : [PORT_NO]           : PORT_NO         --> Client only

CHECK_SHELL=`echo $SHELL|awk -F'/' '{print $NF}'`
SYSTEM=`uname`

KMTUNE=/usr/sbin/kmtune
KCTUNE=/usr/sbin/kctune

INSTALLDIR=XXXX
DBCREATE_FLAG=0

ALTIBASE_USER_ENV=${INSTALLDIR}/conf/altibase_user.env
ALTIBASE_LICENSE=${INSTALLDIR}/conf/license
#echo "Target OS    : $SYSTEM"
#echo "Target SHELL : $CHECK_SHELL"


##############################
# select target Env File.
##############################

setProfile ()
{
    case $CHECK_SHELL in
    csh)
        if [ -f "${HOME}/.cshrc" ]
        then
            profile="${HOME}/.cshrc"
        elif [ -f "${HOME}/.login" ]
        then
            profile="${HOME}/.login"
        else
            profile="${HOME}/.cshrc"
        fi;;
    bash)
        if [ -f "${HOME}/.bash_profile" ]
        then
            profile="${HOME}/.bash_profile"
            elif [ -f "${HOME}/.bash_login" ]
        then
            profile="${HOME}/.bash_login"
        elif [ -f "${HOME}/.profile" ]
        then
            profile="${HOME}/.profile"
        else
            profile="${HOME}/.bash_profile"
        fi;;
    ksh)
        if [ -f "${HOME}/.profile" ]
        then
            profile="${HOME}/.profile"
        elif [ -f "${HOME}/.kshrc" ]
        then
            profile="${HOME}/.kshrc"
        else
            profile="${HOME}/.profile"
        fi;;
    sh)
            profile="${HOME}/.profile";;
    *)
        echo "  Sorry, you have to set above enveroment variables in your shell init file yourself."
        return;;
    esac

}


##############################
# set Linux Env into ${installdir}/conf/altibase_user.env
##############################

exportEnvVariable_Linux ()
{
    if [ $CHECK_SHELL = csh ]
    then
        echo "
setenv ALTIBASE_HOME ${INSTALLDIR}
setenv PATH .:\${ALTIBASE_HOME}/bin:\${PATH}
setenv LD_LIBRARY_PATH .:\${ALTIBASE_HOME}/lib:\${LD_LIBRARY_PATH}
setenv CLASSPATH \${ALTIBASE_HOME}/lib/Altibase.jar:\${CLASSPATH}
        " > $ALTIBASE_USER_ENV
    else
        echo "
ALTIBASE_HOME=${INSTALLDIR};export ALTIBASE_HOME
PATH=\${ALTIBASE_HOME}/bin:\${PATH};export PATH
LD_LIBRARY_PATH=\${ALTIBASE_HOME}/lib:\${LD_LIBRARY_PATH};export LD_LIBRARY_PATH
CLASSPATH=\${ALTIBASE_HOME}/lib/Altibase.jar:\${CLASSPATH};export CLASSPATH
        " > $ALTIBASE_USER_ENV
    fi

	echo "[Linux Env.]
	Target : $ALTIBASE_USER_ENV

	created  ----------------------- Altibase environment setup file.
	added    ----------------------- ALTIBASE_HOME
	added    ----------------------- PATH
	added    ----------------------- LD_LIBRARY_PATH
	added    ----------------------- CLASSPATH
	"
}

##############################
# set AIX Env into ${installdir}/conf/altibase_user.env
##############################

exportEnvVariable_AIX ()
{
    if [ $CHECK_SHELL = csh ]
    then
        echo "
setenv ALTIBASE_HOME ${INSTALLDIR}
setenv PATH .:\${ALTIBASE_HOME}/bin:\${PATH}
setenv LD_LIBRARY_PATH .:\${ALTIBASE_HOME}/lib:\${LD_LIBRARY_PATH}
setenv CLASSPATH \${ALTIBASE_HOME}/lib/Altibase.jar:\${CLASSPATH}
setenv AIXTHREAD_MNRATIO 1:1
setenv AIXTHREAD_SCOPE S
setenv AIXTHREAD_MUTEX_DEBUG OFF
setenv AIXTHREAD_RWLOCK_DEBUG OFF
setenv AIXTHREAD_COND_DEBUG OFF
setenv SPINLOOPTIME 1000
setenv YIELDLOOPTIME 50
setenv MALLOCMULTIHEAP 1
        " > $ALTIBASE_USER_ENV
    else
        echo "
ALTIBASE_HOME=${INSTALLDIR};export ALTIBASE_HOME
PATH=\${ALTIBASE_HOME}/bin:\${PATH};export PATH
LD_LIBRARY_PATH=\${ALTIBASE_HOME}/lib:\${LD_LIBRARY_PATH};export LD_LIBRARY_PATH
CLASSPATH=\${ALTIBASE_HOME}/lib/Altibase.jar:\${CLASSPATH};export CLASSPATH
AIXTHREAD_MNRATIO=1:1;export AIXTHREAD_MNRATIO
AIXTHREAD_SCOPE=S;export AIXTHREAD_SCOPE
AIXTHREAD_MUTEX_DEBUG=OFF;export AIXTHREAD_MUTEX_DEBUG
AIXTHREAD_RWLOCK_DEBUG=OFF;export AIXTHREAD_RWLOCK_DEBUG
AIXTHREAD_COND_DEBUG=OFF;export AIXTHREAD_COND_DEBUG
SPINLOOPTIME=1000;export SPINLOOPTIME
YIELDLOOPTIME=50;export YIELDLOOPTIME
MALLOCMULTIHEAP=1;export MALLOCMULTIHEAP
        " > $ALTIBASE_USER_ENV
    fi

	echo "[AIX Env.]
	Target : $ALTIBASE_USER_ENV
    created  ----------------------- Altibase environment setup file.
	added    ----------------------- ALTIBASE_HOME
	added    ----------------------- PATH
	added    ----------------------- LD_LIBRARY_PATH
	added    ----------------------- CLASSPATH
	added    ----------------------- AIXTHREAD_MNRATIO
	added    ----------------------- AIXTHREAD_SCOPE
	added    ----------------------- AIXTHREAD_MUTEX_DEBUG
	added    ----------------------- AIXTHREAD_RWLOCK_DEBUG
	added    ----------------------- AIXTHREAD_COND_DEBUG
	added    ----------------------- SPINLOOPTIME
	added    ----------------------- YIELDLOOPTIME
	added    ----------------------- MALLOCMULTIHEAP
	"
}

##############################
# set HP-UX Env into ${installdir}/conf/altibase_user.env
##############################

exportEnvVariable_HPUX ()
{
    if [ $CHECK_SHELL = csh ]
    then
        echo "
setenv ALTIBASE_HOME ${INSTALLDIR}
setenv PATH .:\${ALTIBASE_HOME}/bin:\${PATH}
setenv LD_LIBRARY_PATH .:\${ALTIBASE_HOME}/lib:\${LD_LIBRARY_PATH}
setenv CLASSPATH \${ALTIBASE_HOME}/lib/Altibase.jar:\${CLASSPATH}
setenv SHLIB_PATH \${ALTIBASE_HOME}/lib:\${SHLIB_PATH}
setenv PTHREAD_FORCE_SCOPE_SYSTEM 1
setenv _M_ARENA_OPTS 1:8
        " > $ALTIBASE_USER_ENV
    else
        echo "
ALTIBASE_HOME=${INSTALLDIR};export ALTIBASE_HOME
PATH=\${ALTIBASE_HOME}/bin:\${PATH};export PATH
LD_LIBRARY_PATH=\${ALTIBASE_HOME}/lib:\${LD_LIBRARY_PATH};export LD_LIBRARY_PATH
CLASSPATH=\${ALTIBASE_HOME}/lib/Altibase.jar:\${CLASSPATH};export CLASSPATH
SHLIB_PATH=\${ALTIBASE_HOME}/lib:\${SHLIB_PATH};export SHLIB_PATH
PTHREAD_FORCE_SCOPE_SYSTEM=1;export PTHREAD_FORCE_SCOPE_SYSTEM
        " > $ALTIBASE_USER_ENV
    fi

    echo "[HP-UX Env.]
	Target : $ALTIBASE_USER_ENV

    created  ----------------------- Altibase environment setup file.
	added    ----------------------- ALTIBASE_HOME
	added    ----------------------- PATH
	added    ----------------------- LD_LIBRARY_PATH
	added    ----------------------- CLASSPATH
	added    ----------------------- SHLIB_PATH
	added    ----------------------- PTHREAD_FORCE_SCOPE_SYSTEM
	"
}

##############################
# set Sun Env into ${installdir}/conf/altibase_user.env
##############################

exportEnvVariable_SunOS ()
{
    if [ $CHECK_SHELL = csh ]
    then
        echo "
setenv ALTIBASE_HOME ${INSTALLDIR}
setenv PATH .:\${ALTIBASE_HOME}/bin:\${PATH}
setenv LD_LIBRARY_PATH .:\${ALTIBASE_HOME}/lib:\${LD_LIBRARY_PATH}
setenv CLASSPATH \${ALTIBASE_HOME}/lib/Altibase.jar:\${CLASSPATH}
setenv LD_LIBRARY_PATH_64 \${ALTIBASE_HOME}/lib:\${LD_LIBRARY_PATH_64}
        " > $ALTIBASE_USER_ENV
    else
        echo "
ALTIBASE_HOME=${INSTALLDIR};export ALTIBASE_HOME
PATH=\${ALTIBASE_HOME}/bin:\${PATH};export PATH
LD_LIBRARY_PATH=\${ALTIBASE_HOME}/lib:\${LD_LIBRARY_PATH};export LD_LIBRARY_PATH
CLASSPATH=\${ALTIBASE_HOME}/lib/Altibase.jar:\${CLASSPATH};export CLASSPATH
LD_LIBRARY_PATH_64=\${ALTIBASE_HOME}/lib:\${LD_LIBRARY_PATH_64};export LD_LIBRARY_PATH_64
        " > $ALTIBASE_USER_ENV
    fi


    echo "[SunOS Env.]
	Target : $ALTIBASE_USER_ENV

    created  ----------------------- Altibase environment setup file.
	added    ----------------------- ALTIBASE_HOME
	added    ----------------------- PATH
	added    ----------------------- LD_LIBRARY_PATH
	added    ----------------------- LD_LIBRARY_PATH_64
	added    ----------------------- CLASSPATH
	"
}


##############################
# post_install.sh start
##############################

case "$1" in
   setenv)
      setProfile

      if [ $SYSTEM = "Linux" ]
      then
         exportEnvVariable_Linux
      elif [ $SYSTEM = "AIX" ]
      then
          exportEnvVariable_AIX
      elif [ $SYSTEM = "HP-UX" ]
      then
          exportEnvVariable_HPUX
      elif [ $SYSTEM = "SunOS" ]
      then
          exportEnvVariable_SunOS
      fi

      if [ $CHECK_SHELL = csh ]
      then
         if [ $2 = "client" ]
         then
             echo "setenv ALTIBASE_PORT_NO $3" >> $ALTIBASE_USER_ENV
             echo ""
             echo "          added    -----------------------  ALTIBASE_PORT_NO"
         fi

         echo "" >> $profile
         echo "# ALTIBASE_ENV" >> $profile
         echo "source $ALTIBASE_USER_ENV" >> $profile
         source $ALTIBASE_USER_ENV
      else
         if [ $2 = "client" ]
         then
             echo "ALTIBASE_PORT_NO=$3;export ALTIBASE_PORT_NO" >> $ALTIBASE_USER_ENV
             echo ""
             echo "          added    -----------------------  ALTIBASE_PORT_NO"
         fi

         echo "" >> $profile
         echo "# ALTIBASE_ENV" >> $profile
         echo ". $ALTIBASE_USER_ENV" >> $profile
         . $ALTIBASE_USER_ENV
      fi  

      echo " "
      echo "        export ----------------------- 'altibase_user.env'"
      echo "        into '${profile}'"
      echo " "

      echo " "
      echo " "
      echo "=============================================="
      echo "Please perform [re-login] "
      echo "or [source ${profile}] "
      echo "or [. ${profile}]"
      echo "=============================================="
   ;;
   dbcreate)
      if [ $DBCREATE_FLAG -eq 0 ]
      then
         echo "you need to create a database manually."
         echo "shell> server create [DB Character Set][National Character Set]"
      else

         if [ $CHECK_SHELL = csh ]
         then
            source $ALTIBASE_USER_ENV
         else
            . $ALTIBASE_USER_ENV
         fi


         # create DB code

         echo ""
      fi
   ;;
   *)
      echo ""
      echo "Please input option"
      echo "ex) sh post_install.sh setenv   : set env for server]"
      echo "ex) sh post_install.sh setenv client [PORT_NO] : set env for client"
      echo "ex) sh post_install.sh dbcreate : db create"
      echo ""
      echo ""
   ;;
esac



