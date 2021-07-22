#!/bin/sh

# $1 : OS type : LINUX, AIX, SOLARIS, HPUX
# called by $ALTIBASE_DEV/Makefile with ALTI_CFG_OS variable.
CURRENT_DIR="${ALTIBASE_XDB_DEV}/tool/installer/msg/kernel_guide"
TARGET_FILE="${CURRENT_DIR}/kernelSetGuide.txt"


rm -rf ${TARGET_FILE}

case $1 in
    LINUX ) 
       cat ${CURRENT_DIR}/kernelSetGuide_head > ${TARGET_FILE}
       cat ${CURRENT_DIR}/kernelSetGuide_linux >> ${TARGET_FILE}
    ;;
    AIX )
       cat ${CURRENT_DIR}/kernelSetGuide_head > ${TARGET_FILE}
       cat ${CURRENT_DIR}/kernelSetGuide_aix  >> ${TARGET_FILE}
    ;;
    SOLARIS )
       cat ${CURRENT_DIR}/kernelSetGuide_head > ${TARGET_FILE}
       cat ${CURRENT_DIR}/kernelSetGuide_sun >> ${TARGET_FILE}
    ;;
    HPUX )
       cat ${CURRENT_DIR}/kernelSetGuide_head > ${TARGET_FILE}
       cat ${CURRENT_DIR}/kernelSetGuide_hp  >> ${TARGET_FILE}
    ;;
    *)
       echo "Please input OS type."
       echo "(LINUX, AIX, SOLARIS, HPUX)"
       exit 1
    ;;
esac

