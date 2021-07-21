/** 
 *  Copyright (c) 1999~2017, Altibase Corp. and/or its affiliates. All rights reserved.
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU Lesser General Public License, version 3,
 *  as published by the Free Software Foundation.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public License
 *  along with this program. If not, see <http://www.gnu.org/licenses/>.
 */
 

/***********************************************************************
 * $Id$
 **********************************************************************/

/***********************************************************************
 * NAME
 *  ideLog.h
 *
 * DESCRIPTION
 *  This file defines methods for writing to trace log.
 *
 * **********************************************************************/

#ifndef _O_IDELOG_H_
#define _O_IDELOG_H_

#include <ideLogEntry.h>
#include <ideMsgLog.h>
#include <iduShmProcType.h>

class ideMsgLog;
struct idvSQL;

#ifdef ALTIBASE_PRODUCT_XDB
# define IDE_BOOT_LOG_FILE      PRODUCT_PREFIX"altibase_boot_%t.log"
#else
# define IDE_BOOT_LOG_FILE      PRODUCT_PREFIX"altibase_boot.log"
#endif

#define IDE_USER_LOG_FILE       PRODUCT_PREFIX"altibase_user_%p.log"
#define IDE_ERROR_LOG_FILE      PRODUCT_PREFIX"altibase_error.log"
#define IDE_DUMP_LOG_FILE       PRODUCT_PREFIX"altibase_dump.log"
#define IDE_STACK_DUMP_LOG_FILE PRODUCT_PREFIX"altibase_error.trc"

/* PROJ-2581 */
#if defined(ALTIBASE_FIT_CHECK)
#define IDE_FIT_LOG_FILE        PRODUCT_PREFIX"altibase_fit.log"
#endif

#define IDE_MSGLOG_FUNC(body)     ;
#define IDE_MSGLOG_ERROR(a, body) ;

#define IDE_MSGLOG_BODY            idg_MsgLog.logBody

#define IDE_MSGLOG_BEGIN_BLOCK   ""
#define IDE_MSGLOG_END_BLOCK     ""

#define IDE_MSGLOG_SUBBEGIN_BLOCK  "["
#define IDE_MSGLOG_SUBEND_BLOCK    "]"

/* ----------------------------------------------------------------------
 *  BootLog Function (altibase_boot.log)
 * ----------------------------------------------------------------------*/

#define IDE_MSGLOG_TYPE_MSG       0x00000001
#define IDE_MSGLOG_TYPE_RAW       0x00000002
#define IDE_MSGLOG_TYPE_THR       0x00000004
#define IDE_MSGLOG_TYPE_TIMESTAMP 0x00000008
#define IDE_MSGLOG_TYPE_ERROR     0x00000010

/* ------------------------------------------------
 *  ideLogModule 로부터 Module 번호와 레벨을 얻는다.
 * ----------------------------------------------*/

#define IDE_GET_TRC_MODULE(a)  ((a) >> 32)
#define IDE_GET_TRC_LEVEL(a)   ((a) & ID_ULONG(0x00000000FFFFFFFF))

/* ---------------------------------------------
 * TASK-4007 [SM]PBT를 위한 기능 추가
 *
 * Hexa code로 Dump해주는 기능 추가
 * --------------------------------------------- */

/*   SRC는 덤프 대상의 크기, DEST는 덤프되어 저장되는
 * 버퍼의 크기입니다. SRC 크기가 다를 경우에는 LIMIT
 * 까지 출력하고, DEST 크기가 다를 경우에는 아예 출력
 * 하지 않습니다.
 *   SRC 크기가 다를 경우는 어차피 값을 읽는데 문제가
 * 없지만, DEST크기가 다를 경우에는 출력 대상이 되는
 * 버퍼를 신뢰할 수 없기 때문입니다.
 */
#define IDE_DUMP_SRC_LIMIT  ( 64*1024)
#define IDE_DUMP_DEST_LIMIT (256*1024)

// Format flag

/* 개행 공백 등을 통해 BinaryBody를 어떻게 구분해줄지를 설정합니다. */
#define IDE_DUMP_FORMAT_PIECE_MASK       (0x00000001)
#define IDE_DUMP_FORMAT_PIECE_SINGLE     (0x00000000) /* 단일한 한 조각으로 출력합니다. */
#define IDE_DUMP_FORMAT_PIECE_4BYTE      (0x00000001) /* 4Byte단위로 구분합니다.*/

/* 절대주소 또는 상대주소를 출력해줍니다. */
#define IDE_DUMP_FORMAT_ADDR_MASK        (0x00000006)
#define IDE_DUMP_FORMAT_ADDR_NONE        (0x00000000) /* 주소를 출력하지 않습니다. */
#define IDE_DUMP_FORMAT_ADDR_ABSOLUTE    (0x00000002) /* 절대주소를 출력합니다.     */
#define IDE_DUMP_FORMAT_ADDR_RELATIVE    (0x00000004) /* 상대주소를 출력합니다.     */
#define IDE_DUMP_FORMAT_ADDR_BOTH        (0x00000006) /* 절대 상대 모두 출력합니다. */

/* Binary로 데이터를 출력해줍니다. */
#define IDE_DUMP_FORMAT_BODY_MASK        (0x00000008)
#define IDE_DUMP_FORMAT_BODY_NONE        (0x00000000) /* Binary데이터를 출력하지 않습니다. */
#define IDE_DUMP_FORMAT_BODY_HEX         (0x00000008) /* 16진수로 출력합니다. */

/* Character로 데이터를 출력해줍니다. */
#define IDE_DUMP_FORMAT_CHAR_MASK        (0x00000010)
#define IDE_DUMP_FORMAT_CHAR_NONE        (0x00000000) /* Char데이터를 출력하지 않습니다. */
#define IDE_DUMP_FORMAT_CHAR_ASCII       (0x00000010) /* 공백(32) ~ 126 사이의 값들을 \
                                                       * Ascii로 출력해줍니다. */

#define IDE_DUMP_FORMAT_BINARY         ( IDE_DUMP_FORMAT_PIECE_SINGLE | \
                                         IDE_DUMP_FORMAT_ADDR_NONE |    \
                                         IDE_DUMP_FORMAT_BODY_HEX |     \
                                         IDE_DUMP_FORMAT_CHAR_NONE )

#define IDE_DUMP_FORMAT_NORMAL         ( IDE_DUMP_FORMAT_PIECE_4BYTE  |  \
                                         IDE_DUMP_FORMAT_ADDR_RELATIVE | \
                                         IDE_DUMP_FORMAT_BODY_HEX |      \
                                         IDE_DUMP_FORMAT_CHAR_ASCII )

#define IDE_DUMP_FORMAT_VALUEONLY      ( IDE_DUMP_FORMAT_PIECE_SINGLE | \
                                         IDE_DUMP_FORMAT_ADDR_NONE |    \
                                         IDE_DUMP_FORMAT_BODY_HEX |     \
                                         IDE_DUMP_FORMAT_CHAR_ASCII )

#define IDE_DUMP_FORMAT_FULL           ( IDE_DUMP_FORMAT_PIECE_4BYTE  |  \
                                         IDE_DUMP_FORMAT_ADDR_BOTH |     \
                                         IDE_DUMP_FORMAT_BODY_HEX |      \
                                         IDE_DUMP_FORMAT_CHAR_ASCII )

class ideLog
{
    static ideMsgLog    mLogObj [IDE_LOG_MAX_MODULE];
    static const SChar *mProcTypeStr;
    static ULong        mPID;

    static IDE_RC logInternal(ideLogModule aModule, UInt aLevel, const SChar *aFormat, va_list );
    static IDE_RC logInternalLine(ideLogModule aModule, UInt aLevel, const SChar *aFormat, va_list );

    static void formatTemplate( SChar *aOutBuf,
                                UInt aOutBufLen,
                                SChar const *aTemplate );

public:

    static const SChar *mMsgModuleName[];
    /*
     * BUG-32920
     * Daemon으로 작동할 경우 assert 메시지를 altibase_error.log로 출력하도록
     * Redirection
     * DEBUG 모드로 작동할 경우 assert 메시지를 stdout으로 출력함
     */
    static IDE_RC initializeStaticBoot( iduShmProcType aProcType, idBool aDebug );
    /* 프로퍼티 로딩 이전에 초기화 */
    static IDE_RC destroyStaticBoot();   /* 프로퍼티 로딩 이전에 초기화 */
    static IDE_RC initializeStaticModule(idBool aDebug=ID_FALSE); /* 프로퍼티 로딩 이후에 초기화 */
    static IDE_RC destroyStaticModule();

    // BUG-40916
    // initialize mPID, mProcTypeStr
    static IDE_RC initializeStaticForUtil();
    static IDE_RC destroyStaticForUtil();

    static SChar const *getProcTypeStr() { return mProcTypeStr; }
    static ULong  getProcID() { return mPID; }

    /*
     * BUG-34491
     * altibase_error.log가 닫히는 경우가 없도록 따로 처리
     */
    static IDE_RC destroyStaticError();

    static void  logCallStackInternal();

    static void  itoa(SLong aValue, SChar * aString, SInt aMaxStringLength, SInt aRadix, SInt aFormatLength);

    static void  logErrorMsgInternal (ideLogEntry& aLog);
    static void  logWriteErrorMsgInternal (SInt aFD, UInt aLevel);

    static void  flushAllModuleLogs();

    // Single trclog
    static IDE_RC log(UInt aChkFlag, ideLogModule aModule, UInt aLevel, const SChar *aFormat, ... );

    static IDE_RC logLine(UInt aChkFlag, ideLogModule aModule, UInt aLevel, const SChar *aFormat, ... );
    static IDE_RC logCallStack(UInt aChkFlag, ideLogModule aModule, UInt aLevel);
    static IDE_RC logErrorMsg(UInt aChkFlag, ideLogModule aModule, UInt aLevel);
    static IDE_RC logMem(UInt aChkFlag, ideLogModule aModule, UInt aLevel,
                         const UChar* aPtr, UInt aSize );
    static IDE_RC logMem(UInt aChkFlag, ideLogModule aModule, UInt aLevel,
                         const UChar* aPtr, UInt aSize, const SChar* aFormat, ... );

    // Multiple trclog
    static void logMessage(UInt aChkFlag, ideLogModule aModule, UInt aLevel, const SChar *aFormat, ... );
    static void logWrite(SInt aFD, const SChar *aMessage);

    static IDE_RC ideMemToHexStr( const UChar  * aSrcPtr,
                                  UInt           aSrcSize,
                                  UInt           aFormatFlag,
                                  SChar        * aDstPtr,
                                  UInt           aDstSize );

    static ideMsgLog *getMsgLog(ideLogModule aModule) // in special case for HP
    {
        return &mLogObj[aModule];
    }

    // PROJ-2118 Bug Reporting
    static void writeErrorTrace( const SChar * aErrInfo,
                                 const idBool  aAccpetFaultTolerance, /* PROJ-2617 */
                                 const SChar * aFileName,
                                 const UInt    aLineNum,
                                 const SChar * aFormat,
                                 va_list       ap );

    static void writeErrorTraceInternal( UInt          aChkFlag,
                                         ideLogModule  aModule,
                                         UInt          aLevel,
                                         const SChar * aErrInfo,
                                         const SChar * aFileName,
                                         const UInt    aLineNum,
                                         const SChar * aFormat,
                                         va_list       ap );

    static inline PDL_HANDLE getErrFD(void)
    {
        return mLogObj[IDE_ERR].getFD();
    }
    static void  logErrorMgrStackInternalForDebug(UInt aChkFlag, ideLogModule aModule, UInt aLevel);
};

inline IDE_RC ideLog::log(UInt aChkFlag, ideLogModule aModule, UInt aLevel, const SChar *aFormat, ... )
{
#if !defined(ITRON)

    if (aChkFlag != 0)
    {
        va_list     ap;
        va_start (ap, aFormat);
        logInternal(aModule, aLevel, aFormat, ap);
        va_end (ap);
    }

#endif

    return IDE_SUCCESS;
}

// bug-24840 divide xa log
// XA log를 한줄로 남기기 위해 추가된 함수
// logInternal -> logInternalLine 로 호출함수 변경
// 추가된 함수: logLine, logInternalLine, logOpenLine
inline IDE_RC ideLog::logLine(UInt aChkFlag, ideLogModule aModule, UInt aLevel, const SChar *aFormat, ... )
{
#if !defined(ITRON)

    if (aChkFlag != 0)
    {
        va_list     ap;
        va_start (ap, aFormat);
        logInternalLine(aModule, aLevel, aFormat, ap);
        va_end (ap);
    }

#endif

    return IDE_SUCCESS;
}

inline IDE_RC ideLog::logErrorMsg(UInt aChkFlag, ideLogModule aModule, UInt aLevel)
{
    if (aChkFlag != 0)
    {
        ideLogEntry sLog(aChkFlag, aModule, aLevel);
        logErrorMsgInternal(sLog);
        sLog.write();
    }
    return IDE_SUCCESS;
}

inline IDE_RC ideLog::logCallStack(UInt aChkFlag, ideLogModule, UInt)
{
    if (aChkFlag != 0)
    {
        logCallStackInternal();
    }

    return IDE_SUCCESS;
}

#endif /* _O_IDELOG_H_ */

