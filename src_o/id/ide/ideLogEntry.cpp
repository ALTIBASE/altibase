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
 *  ideMsgLog.cpp
 *
 * DESCRIPTION
 *  This file defines ideLogEntry, used for outputing trace log
 *  entries.
 *
 *   !!!!!!!!!!!!!!!!! WARNING !!!!!!!!!!!!!!!!!!
 *   Don't Use IDE_ASSERT() in this file.
 **********************************************************************/

#include <idl.h>
#include <ideLogEntry.h>
#include <ideErrorMgr.h>
#include <ideLog.h>
#include <iduMemMgr.h>
#include <iduProperty.h>
#include <idtContainer.h>

acp_uint64_t ideLogEntry::mLogSerial = 0;

#define IDE_LOGENTRY_INITIAL_SIZE 256
#define IDE_LOGENTRY_EXPAND_SIZE  1024

static void getTimeString(SChar *aBuf, UInt aBufSize)
{
    PDL_Time_Value sTimevalue;
    struct tm      sLocalTime;
    time_t         sTime;

    sTimevalue = idlOS::gettimeofday();
    sTime      = (time_t)sTimevalue.sec();
    idlOS::localtime_r( &sTime, &sLocalTime );

    idlOS::snprintf(aBuf, aBufSize,
                    "%4"ID_UINT32_FMT
                    "/%02"ID_UINT32_FMT
                    "/%02"ID_UINT32_FMT
                    " %02"ID_UINT32_FMT
                    ":%02"ID_UINT32_FMT
                    ":%02"ID_UINT32_FMT"",
                    sLocalTime.tm_year + 1900,
                    sLocalTime.tm_mon + 1,
                    sLocalTime.tm_mday,
                    sLocalTime.tm_hour,
                    sLocalTime.tm_min,
                    sLocalTime.tm_sec);
}

/************************************************************************
 * ideLogEntry helps logging a trace logging message
 ***********************************************************************/

ideLogEntry::ideLogEntry( UInt aChkFlag,
                          ideLogModule aModule,
                          UInt aLevel,
                          acp_bool_t aNewLine )
    :mMsgLog(ideLog::getMsgLog(aModule)),
     mChkFlag(aChkFlag),
     mLevel(aLevel),
     mLength(0),
     mIsTailless(ACP_FALSE),
     mNewLine(aNewLine),
     mHexDump(NULL)
{
    logOpen();
}

ideLogEntry::ideLogEntry( ideMsgLog *aMsgLog,
                          UInt aChkFlag,
                          UInt aLevel,
                          acp_bool_t aNewLine )
    :mMsgLog(aMsgLog),
     mChkFlag(aChkFlag),
     mLevel(aLevel),
     mLength(0),
     mIsTailless(ACP_FALSE),
     mNewLine(aNewLine),
     mHexDump(NULL)
{
    logOpen();
}

ideLogEntry::~ideLogEntry()
{
    IDE_DASSERT(mHexDump == NULL);
}

IDE_RC ideLogEntry::append( const acp_char_t *aStr )
{
    acp_size_t sStrLen;

    if ( mChkFlag != 0 )
    {
        sStrLen = acpCStrLen(aStr, ACP_ULONG_MAX);
        IDE_TEST( ACP_RC_NOT_SUCCESS( acpCStrCpy( mBuffer + mLength,
                                                  IDE_MESSAGE_SIZE - mLength,
                                                  aStr,
                                                  sStrLen ) ) );
        mLength += sStrLen;
    }
    else
    {
        /* do nothing */
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;    
    
    mLength = IDE_MESSAGE_SIZE;

    return IDE_FAILURE;
}

IDE_RC ideLogEntry::appendForce( const acp_char_t *aStr )
{
    acp_size_t sStrLen;

    if ( mChkFlag != 0 )
    {
        sStrLen = acpCStrLen(aStr, IDE_MESSAGE_SIZE - 1 );

        if ( sStrLen >= IDE_MESSAGE_SIZE - mLength )
        {
            /* -1 for null terminator */
            mLength = IDE_MESSAGE_SIZE - sStrLen - 1;
        }

        IDE_TEST( ACP_RC_NOT_SUCCESS( acpCStrCpy( mBuffer + mLength,
                                                  IDE_MESSAGE_SIZE - mLength,
                                                  aStr,
                                                  sStrLen ) ) );
        mLength += sStrLen;
    }
    else
    {
        /* do nothing */
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    mLength = IDE_MESSAGE_SIZE;

    return IDE_FAILURE;
}



IDE_RC ideLogEntry::appendArgs( const acp_char_t *aFormat, va_list aArgs )
{
    va_list    sArgsCopy;
    acp_size_t sWritten;

    if ( mChkFlag != 0 )
    {
        if ( IDE_MESSAGE_SIZE - mLength > 0 )
        {
            va_copy( sArgsCopy, aArgs );

            sWritten = vsnprintf( mBuffer + mLength,
                                  IDE_MESSAGE_SIZE - mLength,
                                  aFormat,
                                  sArgsCopy );

            va_end( sArgsCopy );

            mLength = IDL_MIN( mLength + sWritten, IDE_MESSAGE_SIZE );
        }
    }
    else
    {
        /* do nothing */
    }

    return IDE_SUCCESS;
}

void ideLogEntry::logOpen()
{
    SChar sTimeBuf[128];
    SChar const *sFormat;

    mLength = 0;

    getTimeString(sTimeBuf, ID_SIZEOF( sTimeBuf ));

    (void)appendForce(IDE_MSGLOG_BEGIN_BLOCK);

    if ( IDL_LIKELY_TRUE( mNewLine ) )
    {
        sFormat =
            IDE_MSGLOG_SUBBEGIN_BLOCK"%s %llX"IDE_MSGLOG_SUBEND_BLOCK
            IDE_MSGLOG_SUBBEGIN_BLOCK"PID:%llu"IDE_MSGLOG_SUBEND_BLOCK
            IDE_MSGLOG_SUBBEGIN_BLOCK"Thread-%llu" IDE_MSGLOG_SUBEND_BLOCK
#ifdef ALTI_CFG_OS_LINUX
            IDE_MSGLOG_SUBBEGIN_BLOCK"LWP-%d" IDE_MSGLOG_SUBEND_BLOCK
#endif
            "%s\n";
    }
    else
    {
        // bug-24840 divide xa log
        // XA log를 한줄로 남기기 위해 추가
        sFormat = 
            IDE_MSGLOG_SUBBEGIN_BLOCK"%s %llX"IDE_MSGLOG_SUBEND_BLOCK
            IDE_MSGLOG_SUBBEGIN_BLOCK"PID:%llu"IDE_MSGLOG_SUBEND_BLOCK
            IDE_MSGLOG_SUBBEGIN_BLOCK"Thread-%llu" IDE_MSGLOG_SUBEND_BLOCK
#ifdef ALTI_CFG_OS_LINUX
            IDE_MSGLOG_SUBBEGIN_BLOCK"LWP-%d" IDE_MSGLOG_SUBEND_BLOCK
#endif
            "%s ";
    }

    mLength = idlOS::snprintf(mBuffer, IDE_MESSAGE_SIZE,
                              sFormat,
                              sTimeBuf,
                              getLogSerial(),
                              ideLog::getProcID(),
                              idtContainer::getSysThreadNumber(), /*BUG-45045*/
#ifdef ALTI_CFG_OS_LINUX
                              idtContainer::getSysLWPNumber(),    /*BUG-45181*/
#endif
                              ideLog::getProcTypeStr());
}

IDE_RC ideLogEntry::logClose()
{
    if ( mChkFlag )
    {
        if ( IDL_LIKELY_TRUE( !mIsTailless ) )
        {
            IDE_TEST( IDE_SUCCESS != this->appendForce("\n"IDE_MSGLOG_END_BLOCK) );
        }
        else
        {
            /* Nothing to do */
        }

        IDE_TEST( IDE_SUCCESS != mMsgLog->logBody(mBuffer, mLength) );

        if(mHexDump != NULL)
        {
            IDE_TEST( IDE_SUCCESS != mMsgLog->logBody(mHexDump, mDumpLength) );
            IDE_TEST( IDE_SUCCESS != iduMemMgr::free(mHexDump) );
            mHexDump = NULL;
        }
        else
        {
            /* do nothing */
        }
    }
    else
    {
        /* do nothing */
    }
    return IDE_SUCCESS;

    IDE_EXCEPTION_END;
    if(mHexDump != NULL)
    {
        (void)iduMemMgr::free(mHexDump);
        mHexDump = NULL;
    }
    return IDE_FAILURE;
}

IDE_RC ideLogEntry::dumpHex(const UChar*    aPtr,
                            const UInt      aSize,
                            const UInt      aFormatFlag)
{
    UInt            i;
    UInt            j;
    UInt            k;
    UInt            sLineSize;
    UInt            sBlockSize;
    acp_char_t*     sDstPtr;
    acp_size_t      sDstSize;
    acp_size_t      sWrittenLength;
    acp_size_t      sSize;

    /* ==========================================
     * Parameter Validation
     * ========================================== */

    IDE_TEST( aPtr             == NULL );
    sSize = IDL_MIN(IDE_DUMP_SRC_LIMIT, aSize);

    mDumpLength = ID_SIZEOF( SChar ) * IDE_DUMP_DEST_LIMIT;
    IDE_TEST_RAISE( iduMemMgr::calloc( IDU_MEM_ID, 1,
                                       mDumpLength,
                                       (void**)&mHexDump )
                    != IDE_SUCCESS, err_fail_alloc );

    /* ==========================================
     * Piece Format
     * ========================================== */

    switch( aFormatFlag & IDE_DUMP_FORMAT_PIECE_MASK )
    {
    case IDE_DUMP_FORMAT_PIECE_4BYTE:
        sLineSize  = 32;
        sBlockSize  = 4;
        break;
    case IDE_DUMP_FORMAT_PIECE_SINGLE:
    default:
        sLineSize  = sSize;
        sBlockSize = sSize;
        break;
    }

    sWrittenLength  = 0;
    sDstSize        = mDumpLength;
    sDstPtr         = mHexDump;

    /* ==========================================
     * Converting
     * ========================================== */
    // Loop를 돌며 한줄씩 출력해준다. 한줄당 LineSize만큼 출력한다.
    for( i = 0 ; i < sSize; i += sLineSize )
    {
        /* ==========================================
         * Address Format
         * ========================================== */
        switch( aFormatFlag & IDE_DUMP_FORMAT_ADDR_MASK )
        {
        case IDE_DUMP_FORMAT_ADDR_NONE:
            sWrittenLength = 0;
            break;
        case IDE_DUMP_FORMAT_ADDR_ABSOLUTE:
            sWrittenLength = idlOS::snprintf( sDstPtr, sDstSize,
                    "%016"ID_xPOINTER_FMT": ",
                    aPtr + i);
            break;
        case IDE_DUMP_FORMAT_ADDR_RELATIVE:
            sWrittenLength = idlOS::snprintf( sDstPtr, sDstSize,
                                 "%08"ID_xINT32_FMT"| ", i);
            break;
        case IDE_DUMP_FORMAT_ADDR_BOTH:
            sWrittenLength = idlOS::snprintf( sDstPtr, sDstSize,
                                 "%016"ID_xPOINTER_FMT": "
                                 "%08"ID_xINT32_FMT"| ",
                                 aPtr + i, i);
            break;
        default:
            sWrittenLength = 0;
            break;
        }

        sDstPtr     += sWrittenLength;
        sDstSize    -= sWrittenLength;

        /* ==========================================
         * Body Format
         * ========================================== */
        switch( aFormatFlag & IDE_DUMP_FORMAT_BODY_MASK )
        {
        case IDE_DUMP_FORMAT_BODY_HEX:
            for( j = 0 ; ( j < sLineSize ) && ( i + j < sSize ) ; j += sBlockSize )
            {
                //Block내 값들을 하나하나 출력한다. 전달받은 메모리 주소가
                //Align 맞는다는 보장이 없기 때문에 한바이트씩 찍어준다.
                for( k = 0 ;
                     ( k < sBlockSize ) &&
                     ( j < sLineSize )  &&
                     ( i + j + k < sSize ) ;
                     k ++)
                {
                    sWrittenLength = idlOS::snprintf( sDstPtr, sDstSize,
                                         "%02"ID_xINT32_FMT, aPtr[ i + j + k] );
        
                    sDstPtr     += sWrittenLength;
                    sDstSize    -= sWrittenLength;
                }

                // BlockSize만큼 HexStr을 출력했으며,
                // Line의 마지막이 아닐 경우, 공백 출력
                if ( ( k == sBlockSize ) && ( j != sLineSize ) )
                {
                    sWrittenLength = idlOS::snprintf( sDstPtr, sDstSize, " " );
                    sDstPtr     += sWrittenLength;
                    sDstSize    -= sWrittenLength;
                }
            }
            break;
        case IDE_DUMP_FORMAT_BODY_NONE:
            break;
        default:
            break;
        }


        /* ==========================================
         * Character Format
         * ========================================== */
        switch( aFormatFlag & IDE_DUMP_FORMAT_CHAR_MASK )
        {
        case IDE_DUMP_FORMAT_CHAR_ASCII:
            sWrittenLength = idlOS::snprintf( sDstPtr, sDstSize, "; " );
            sDstPtr     += sWrittenLength;
            sDstSize    -= sWrittenLength;

            for( j = 0 ; ( j < sLineSize ) && ( i + j < sSize ) ; j ++ )
            {
                //공백(32)와 ~(126)사이의 값들은 Ascii로 출력가능한 것들
                //이다. 그런 것들만 찍어준다.
                sWrittenLength = idlOS::snprintf( sDstPtr, sDstSize,
                                     "%c",
                                     ( isprint( aPtr[ i + j] ) != 0 )
                                     ? aPtr[ i + j] : '.' );
                sDstPtr     += sWrittenLength;
                sDstSize    -= sWrittenLength;
            }
            break;
        case IDE_DUMP_FORMAT_CHAR_NONE:
            break;
        default:
            break;
        }

        // aSrc의 마지막이 아닐 경우, 개행함
        if ( (i + sLineSize) < sSize )
        {
            sWrittenLength = idlOS::snprintf( sDstPtr, sDstSize, "\n" );
            sDstPtr     += sWrittenLength;
            sDstSize    -= sWrittenLength;
        }
    }
            
    /* 마지막에 개행 추가 */
    (void)idlOS::snprintf( sDstPtr, sDstSize, "\n" );

    mDumpLength -= (sDstSize - 1); /* BUG-47898 */

    return IDE_SUCCESS;

    IDE_EXCEPTION( err_fail_alloc );
    {
        // 메모리 할당 실패
        // Heap 영역 메모리 부족으로 Dump결과를 저장할 버퍼를 확보하지 못하였습니다.
        appendFormat( "%s", ideGetErrorMsg(ideGetErrorCode()) );
    }
    // fix BUG-29682 IDE_EXCEPTION_END가 잘못되어 무한루프가 있습니다.
    IDE_EXCEPTION_END;
    mHexDump = NULL;
    mDumpLength = 0;

    return IDE_FAILURE;
}
