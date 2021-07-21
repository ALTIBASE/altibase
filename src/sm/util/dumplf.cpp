/** 
 *  Copyright (c) 1999~2017, Altibase Corp. and/or its affiliates. All rights reserved.
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU Affero General Public License, version 3,
 *  as published by the Free Software Foundation.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU Affero General Public License for more details.
 *
 *  You should have received a copy of the GNU Affero General Public License
 *  along with this program. If not, see <http://www.gnu.org/licenses/>.
 */
 

/***********************************************************************
 * $Id: dumplf.cpp 91224 2021-07-14 05:36:12Z minku.kang $
 **********************************************************************/

#include <idl.h>
#include <ideErrorMgr.h>
#include <smr.h>
#include <smrLogFileDump.h>
#include <smxDef.h>

typedef struct fdEntry
{
    SChar  sName[SM_MAX_FILE_NAME+1];
}fdEntry;

typedef struct gTableInfo
{
    smOID         mTableOID;

    ULong         mInserLogCnt;
    ULong         mUpdateLogCnt;
    ULong         mDeleteLogCnt;
} gTableInfo;

#define  TABLEOID_HASHTABLE_SIZE   (233)  // 소수중 그냥 고른 수

SChar      * gLogFileName     = NULL;
SChar      * gStrLSN          = NULL;     // BUG-44361
smTID        gTID             = 0;        // 특정 TID만 출력할것인가?
idBool       gDisplayValue    = ID_TRUE;  // Log의 Value부분을 출력할것인가?
idBool       gDisplayLogTypes = ID_FALSE; // LogType들을 출력한다.
idBool       gDisplayHeader   = ID_TRUE;
idBool       gInvalidArgs     = ID_FALSE;
/*BUG-42787  logfile를 분석하여 DML 에 대한 수치가 제공되는 Tool 제공  */
idBool       gGroupByTableOID = ID_FALSE; // TableOID로 GROUP 해서 출력할것인가.
idBool       gPathSet         = ID_FALSE; // 특정 경로가 설정되어 있는가.
iduHash      gTableInfoHash;
smLSN        gLSN;                        // BUG-44361 특정 LSN부터 DML 통계 확인
SChar        gPath[SM_MAX_FILE_NAME+1] = {"", };
ULong        gFileIdx      = 0;      
ULong        gInserLogCnt  = 0;
ULong        gUpdateLogCnt = 0;
ULong        gDeleteLogCnt = 0;
ULong        gCommitLogCnt = 0;
ULong        gAbortLogCnt  = 0;
ULong        gInvalidLog   = 0;
ULong        gInvalidFile  = 0;
ULong        gHashItemCnt  = 0;

ULong        gInserLogCnt4Debug  = 0;
ULong        gUpdateLogCnt4Debug = 0;
ULong        gDeleteLogCnt4Debug = 0;
ULong        gHashItemCnt4Debug  = 0;

smrLogFileDump   gLogFileDump;

void   usage();                   // 사용법 출력
IDE_RC initProperty();            // Dump하는데 필요한 Property 초기화
void   parseArgs( int    &aArgc,  // 입력된 인자를 Parsing한다.
                  char **&aArgv );
void   showCopyRight(void);       // Copy right 출력한다.
IDE_RC parseLSN( smLSN * aLSN, const char * aStrLSN );  // BUG-44361 LSN Parse

// 조건에 따라 로그를 Dump한다.
IDE_RC dumpLogFile();

// Log Header를 Dump한다.
IDE_RC dumpLogHead( UInt         aOffsetInFile,
                    smrLogHead * aLogHead,
                    idBool       aIsCompressed );

// Log Body를 Dump한다.
IDE_RC dumpLogBody( UInt         aLogType,
                    smrLogHead * aLogHead,
                    SChar *      aLogPtr );

// Disk Log를 Dump한다.
void dumpDiskLog( SChar         * aLogBuffer,
                  UInt            aAImgSize,
                  UInt          * aOPType );

// dumplf에서 다루어야 하는 LogType들을 모두 출력한다.
void displayLogTypes();

void usage()
{
    idlOS::printf( "\n%-6s:  dumplf {-f log_file} [-t transaction_id] [-s] [-l] [-S LSN] [-F path] [-g]\n", "Usage" );

    idlOS::printf( " %-4s : %s\n", "-f", "specify log file name" );
    idlOS::printf( " %-4s : %s\n", "-t", "specify transaction id" );
    idlOS::printf( " %-4s : %s\n", "-s", "silence log value" );
    idlOS::printf( " %-4s : %s\n", "-l", "display log types only" );
    idlOS::printf( " %-4s : %s\n", "-S", "display DML statistic from specified LSN.\n"
                                 "        example of LSN) 1,1000 or 0,0" );
    idlOS::printf( " %-4s : %s\n", "-F", "should be combined with -S \n"
                                 "        specify the path of logs. \n"
                                 "        If not specified, $ALTIBASE_HOME/logs by default."); 
    idlOS::printf( " %-4s : %s\n", "-g", "should be combined with -S \n" 
                                 "        display the separate statistic list for TableOIDs" );
    idlOS::printf( "\n" );
}


IDE_RC initProperty()
{
    /* TASK-4007 [SM] PBT를 위한 기능 추가
     * altibase.properties와 상관 없이 logfile을 읽을 수 있어야 한다.
     * 따라서 반드시 설정해야하는 mLogFileSize만 추기화 한다. */
    smuProperty::mLogFileSize = ID_ULONG_MAX;

    return IDE_SUCCESS;
}


void parseArgs( int &aArgc, char **&aArgv )
{
    SInt sOpr;

    sOpr = idlOS::getopt( aArgc, aArgv, "f:t:slxS:F:g" );

    if(sOpr != EOF)
    {
        do
        {
            switch( sOpr )
            {
            case 'f':
                gLogFileName     = optarg;
                break;
            case 't':
                gTID             = (smTID)idlOS::atoi( optarg );
                break;
            case 's':
                gDisplayValue    = ID_FALSE;
                break;
            case 'l':
                gDisplayLogTypes = ID_TRUE;
                break;
            case 'x':
                gDisplayHeader   = ID_FALSE;
                break;
            case 'S':
                gStrLSN          = optarg;  // BUG-44361
                break;
            case 'F':
                idlOS::strcpy(gPath, optarg);
                gPathSet = ID_TRUE;
                break;
            case 'g':
                gGroupByTableOID = ID_TRUE;
                break; 
            default:
                gInvalidArgs     = ID_TRUE;
                break;
            }
        }
        while( (sOpr = idlOS::getopt(aArgc, aArgv, "f:t:slxS:F:g")) != EOF );
    }
    else
    {
        gInvalidArgs = ID_TRUE;
    }
}


/***********************************************************************
     Log의 Head를 출력한다.

     [IN] aOffset  - 출력할 Log의 로그파일상의 Offset
     [IN] aLogHead - 출력할 Log의 Head
     [IN] aIsCompressed - 압축되었는지 여부
 **********************************************************************/
IDE_RC dumpLogHead( UInt         aOffsetInFile,
                    smrLogHead * aLogHead,
                    idBool       aIsCompressed)
{
    idBool sIsBeginLog = ID_FALSE;
    idBool sIsReplLog  = ID_FALSE;
    idBool sIsSvpLog   = ID_FALSE;
    idBool sIsFullXLog = ID_FALSE;
    idBool sIsDummyLog = ID_FALSE;
    smLSN  sLSN;

    ACP_UNUSED( aOffsetInFile );

    if( ( smrLogHeadI::getFlag( aLogHead ) & SMR_LOG_BEGINTRANS_MASK )
        == SMR_LOG_BEGINTRANS_OK )
    {
        sIsBeginLog = ID_TRUE;
    }

    if( ( smrLogHeadI::getFlag( aLogHead ) & SMR_LOG_TYPE_MASK )
        == SMR_LOG_TYPE_NORMAL )
    {
        sIsReplLog = ID_TRUE;
    }

    if( ( smrLogHeadI::getFlag( aLogHead ) & SMR_LOG_SAVEPOINT_MASK )
        == SMR_LOG_SAVEPOINT_OK )
    {
        sIsSvpLog = ID_TRUE;
    }

    if( ( smrLogHeadI::getFlag( aLogHead ) & SMR_LOG_FULL_XLOG_MASK )
        == SMR_LOG_FULL_XLOG_OK )
    {
        sIsFullXLog = ID_TRUE;
    }
    /* BUG-35392 */
     if( ( smrLogHeadI::getFlag( aLogHead ) & SMR_LOG_DUMMY_LOG_MASK )
        == SMR_LOG_DUMMY_LOG_OK )
    {
        sIsDummyLog = ID_TRUE;
    }

    sLSN = smrLogHeadI::getLSN( aLogHead );
    IDE_DASSERT( sLSN.mOffset == aOffsetInFile );
    /* BUG-31053 The display format of LSN lacks consistency in Utility.
     * Logfile 하나만으로는 이 LogRecord의 LFGID, LogFileNumber를 알 수
     * 없기 때문에 Offset만 Format에 맞춰 출력합니다. */
    idlOS::printf("LSN=<%"ID_UINT32_FMT", %"ID_UINT32_FMT">, "
                  "COMP: %s, DUMMY: %s, MAGIC: %"ID_UINT32_FMT", TID: %"ID_UINT32_FMT", "
                  "BE: %s, REP: %s, FXLOG: %s, ISVP: %s, ISVP_DEPTH: %"ID_UINT32_FMT,
                  sLSN.mFileNo,
                  sLSN.mOffset,
                  (aIsCompressed == ID_TRUE)?"Y":"N",
                  (sIsDummyLog   == ID_TRUE)?"Y":"N", /* BUG-35392 */
                  smrLogHeadI::getMagic(aLogHead),
                  smrLogHeadI::getTransID(aLogHead),
                  (sIsBeginLog == ID_TRUE)?"Y":"N",
                  (sIsReplLog == ID_TRUE)?"Y":"N",
                  (sIsFullXLog == ID_TRUE)?"Y":"N",
                  (sIsSvpLog == ID_TRUE)?"Y":"N",
                  // BUG-28266 DUMPLF으로 SAVEPOINT NAME을 볼 수 있으면 좋겠습니다
                  smrLogHeadI::getReplStmtDepth(aLogHead));

    idlOS::printf(" PLSN=<%"ID_UINT32_FMT", "
                  "%"ID_UINT32_FMT">, LT: %s< %"ID_UINT32_FMT" >, SZ: %"ID_UINT32_FMT" ",
                  smrLogHeadI::getPrevLSNFileNo(aLogHead),
                  smrLogHeadI::getPrevLSNOffset(aLogHead),
                  smrLogFileDump::getLogType( smrLogHeadI::getType(aLogHead) ),
                  smrLogHeadI::getType(aLogHead),
                  smrLogHeadI::getSize(aLogHead));

    return IDE_SUCCESS;
}


/***********************************************************************
    Log의 Body를 출력한다

    [IN] aLogType - 출력할 로그의 Type
    [IN] aLogBody - 출력할 로그의 Body
 **********************************************************************/
IDE_RC dumpLogBody( UInt aLogType, smrLogHead * aLogHead, SChar *aLogPtr )
{
    UInt                 i;
    smrBeginChkptLog     sBeginChkptLog;
    smrNTALog            sNTALog;
    smrUpdateLog         sUpdateLog;
    smrDiskLog           sDiskLog;
    smrDiskNTALog        sDiskNTA;
    smrDiskRefNTALog     sDiskRefNTA;
    smrDiskCMPSLog       sDiskCMPLog;
    smrCMPSLog           sCMPLog;
    smrFileBeginLog      sFileBeginLog;
    smSCN                sSCN;
    SChar*               sLogBuffer = NULL;
    smrTransCommitLog    sCommitLog;
    smrXaPrepareLog      sPrepareLog;
    smrXaPrepareReqLog   sPrepareReqLog;
    smrXaEndLog          sEndLog;
    smrXaSegsLog         sSegLog;
    smrXaStartReqLog     sStartReqLog;
    scPageID             sBeforePID;
    scPageID             sAfterPID;
    smrLobLog            sLobLog;
    time_t               sCommitTIME;
    struct tm            sCommit;
    ULong                slBeforeValue;
    ULong                slAfterValue1;
    ULong                slAfterValue2;
    smrTableMetaLog      sTableMetaLog;
    smrDDLStmtMeta       sDDLStmtMeta;
    smrTBSUptLog         sTBSUptLog;
    UInt                 sSPNameLen;
    SChar                sSPName[SMX_MAX_SVPNAME_SIZE];
    SChar*               sTempBuf;
    UInt                 sState     = 0;
    UInt                 sTempUInt  = 0;
    UChar*               sLogPtr; 
    UInt                 sColCnt;
    UInt                 sPieceCnt;
    smOID                sVCPieceOID;
    UShort               sBefFlag;
    UShort               sAftFlag;
    UInt                 sFullXLogSize;
    UInt                 sPrimaryKeySize;
    /* BUG-47525 Group Commit */
    smTID                sTID;
    smTID                sGlobalTID;
    smrTransGroupCommitLog sGCLog;
    UInt                   sGCCnt;
    SChar                * sGCCommitSCNBuffer;
    smSCN                  sCommitSCN;
    UInt                   sLogSize;
    idBool                 sIsWithCommitSCN = ID_FALSE;
    UChar                  sXidString[SMR_XID_DATA_MAX_LEN];

    /* BUG-30882 dumplf does not display the value of MRDB log.
     *
     * 해당 메모리 영역을 Dump한 결과값을 저장할 버퍼를 확보합니다.
     * Stack에 선언할 경우, 이 함수를 통해 서버가 종료될 수 있으므로
     * Heap에 할당을 시도한 후, 성공하면 기록, 성공하지 않으면 그냥
     * return합니다. */
    IDE_TEST( iduMemMgr::calloc( IDU_MEM_ID, 1,
                                 ID_SIZEOF( SChar ) * IDE_DUMP_DEST_LIMIT,
                                 (void**)&sTempBuf )
              != IDE_SUCCESS );
    sState = 1;


    switch(aLogType)
    {
        case SMR_DLT_REDOONLY:
        case SMR_DLT_UNDOABLE:
            idlOS::memcpy( &sDiskLog, aLogPtr, ID_SIZEOF( smrDiskLog ) );

            if( sDiskLog.mTableOID != SM_NULL_OID )
            {
                idlOS::printf("RdSz: %"ID_UINT32_FMT", DMIOff: %"ID_UINT32_FMT","
                              " TableOID: %"ID_vULONG_FMT", ContType: %"ID_UINT32_FMT"\n",
                              sDiskLog.mRedoLogSize,
                              sDiskLog.mRefOffset,
                              sDiskLog.mTableOID,
                              sDiskLog.mContType);
            }
            else
            {
                idlOS::printf("RdSz: %"ID_UINT32_FMT", DMIOff: %"ID_UINT32_FMT","
                              " ContType: %"ID_UINT32_FMT"\n",
                              sDiskLog.mRedoLogSize,
                              sDiskLog.mRefOffset,
                              sDiskLog.mContType);
            }

            sLogBuffer = ((SChar*)aLogPtr + SMR_LOGREC_SIZE(smrDiskLog));

            dumpDiskLog( sLogBuffer,
                         sDiskLog.mRedoLogSize,
                         NULL );
            break;

        case SMR_DLT_NTA:

            idlOS::memcpy( &sDiskNTA, aLogPtr, ID_SIZEOF( smrDiskNTALog ) );

            sLogBuffer = ((SChar*)aLogPtr +
                          SMR_LOGREC_SIZE(smrDiskNTALog));

            dumpDiskLog( sLogBuffer,
                         sDiskNTA.mRedoLogSize,
                         &sDiskNTA.mOPType );
            break;

        case SMR_DLT_REF_NTA:

            idlOS::memcpy( &sDiskRefNTA, aLogPtr, ID_SIZEOF( smrDiskRefNTALog ) );

            sLogBuffer = ((SChar*)aLogPtr +
                          SMR_LOGREC_SIZE(smrDiskRefNTALog));

            idlOS::printf("RdSz: %"ID_UINT32_FMT", DMIOff: %"ID_UINT32_FMT
                          ", SPACEID: %"ID_UINT32_FMT
                          ", ## NTA: %s< %"ID_UINT32_FMT" >\n", 
                          sDiskRefNTA.mRedoLogSize,
                          sDiskRefNTA.mRefOffset,
                          sDiskRefNTA.mSpaceID,
                          smrLogFileDump::getDiskOPType( 
                                sDiskRefNTA.mOPType ),
                          sDiskRefNTA.mOPType );

            dumpDiskLog( sLogBuffer,
                         sDiskRefNTA.mRedoLogSize,
                         NULL );
            break;

        case SMR_LT_TBS_UPDATE:

            // BUGBUG
            // 테이블스페이스 Update 로그타입을 출력한다.
            // 현재는 Extend DBF에 대해서만 자세히 출력하고, 나머지 로그타입에 대해>서는
            // 로그타입만 출력한다. 모든 로그타입에 대해서 자세히 출력할 필요가 있다.
            idlOS::memcpy( &sTBSUptLog, aLogPtr, ID_SIZEOF( smrTBSUptLog ) );

            idlOS::printf("SPACEID: %"ID_UINT32_FMT"\n", sTBSUptLog.mSpaceID);

            sLogBuffer = ((SChar*)aLogPtr + SMR_LOGREC_SIZE(smrTBSUptLog));

            /* PROJ-1923 */
            switch( (sctUpdateType)sTBSUptLog.mTBSUptType )
            {
                case SCT_UPDATE_MRDB_CREATE_TBS:
                    idlOS::printf("## TBS Upate Type: %s < %"ID_UINT32_FMT" >\n"
                                  "# BEFORE - SZ: %"ID_UINT32_FMT"\n"
                                  "# AFTER  - SZ: %"ID_UINT32_FMT"\n",
                                  smrLogFileDump::getTBSUptType( sTBSUptLog.mTBSUptType ),
                                  sTBSUptLog.mTBSUptType,
                                  sTBSUptLog.mBImgSize, 
                                  sTBSUptLog.mAImgSize );

                    // sChkptPathCount를 읽어온다.
                    sTempUInt = 0;
                    idlOS::memcpy( (void *)&sTempUInt,
                                   (UChar *)aLogPtr + SMR_LOGREC_SIZE(smrTBSUptLog) +
                                   sTBSUptLog.mBImgSize + ID_SIZEOF(smiTableSpaceAttr),
                                   ID_SIZEOF(UInt) );
                    idlOS::printf("  Check Point Path Count : %"ID_UINT32_FMT"\n", sTempUInt);

                    // Check Point Path 출력
                    for( i = 0 ; i < sTempUInt ; i++ )
                    {
                        sSPNameLen = 0;
                        idlOS::memcpy( sTempBuf,
                                       (UChar *)aLogPtr +
                                       SMR_LOGREC_SIZE(smrTBSUptLog) + sTBSUptLog.mBImgSize +
                                       ID_SIZEOF(smiTableSpaceAttr) + ID_SIZEOF(UInt) + 
                                       ((i * ID_SIZEOF(smiChkptPathAttrList)) + ID_SIZEOF(smiNodeAttrType) + ID_SIZEOF(scSpaceID)),
                                       SMI_MAX_CHKPT_PATH_NAME_LEN + 1 );
                        sSPNameLen = idlOS::strlen( sTempBuf );

                        sTempBuf[ sSPNameLen ] = '\0';

                        idlOS::printf("  Check Point Path [ %"ID_UINT32_FMT" ] : %s\n",
                                      i,
                                      sTempBuf);
                    } // end of for

                    // After Value 전체 출력
                    ideLog::ideMemToHexStr( (UChar *)aLogPtr + SMR_LOGREC_SIZE(smrTBSUptLog) + sTBSUptLog.mBImgSize,
                                            sTBSUptLog.mAImgSize,
                                            IDE_DUMP_FORMAT_BINARY,
                                            sTempBuf,
                                            IDE_DUMP_DEST_LIMIT );
                    idlOS::printf("  VALUE: 0x%s\n", sTempBuf);

                    break;

                case SCT_UPDATE_DRDB_CREATE_TBS:
                    idlOS::printf("## TBS Upate Type: %s < %"ID_UINT32_FMT" >,"
                                  "BEFORE - SZ: %"ID_UINT32_FMT", AFTER  - SZ: %"ID_UINT32_FMT"\n",
                                  smrLogFileDump::getTBSUptType( sTBSUptLog.mTBSUptType ),
                                  sTBSUptLog.mTBSUptType,
                                  sTBSUptLog.mBImgSize, 
                                  sTBSUptLog.mAImgSize );

                    ideLog::ideMemToHexStr( (UChar *)aLogPtr + SMR_LOGREC_SIZE(smrTBSUptLog) + sTBSUptLog.mBImgSize,
                                            sTBSUptLog.mAImgSize,
                                            IDE_DUMP_FORMAT_BINARY,
                                            sTempBuf,
                                            IDE_DUMP_DEST_LIMIT );
                    idlOS::printf("VALUE: 0x%s\n", sTempBuf);

                    idlOS::memcpy( (void *)&sSPNameLen,
                                   (UChar *)aLogPtr + SMR_LOGREC_SIZE(smrTBSUptLog) +
                                   sTBSUptLog.mBImgSize +
                                   ID_SIZEOF(smiNodeAttrType) +
                                   ID_SIZEOF(scSpaceID) + SMI_MAX_TABLESPACE_NAME_LEN + 1 + 1, // + 1 + padding
                                   ID_SIZEOF(UInt) );

                    ideLog::ideMemToHexStr( (UChar *)aLogPtr +
                                            SMR_LOGREC_SIZE(smrTBSUptLog) +
                                            sTBSUptLog.mBImgSize +
                                            ID_SIZEOF(smiNodeAttrType) +
                                            ID_SIZEOF(scSpaceID),
                                            sSPNameLen,
                                            IDE_DUMP_FORMAT_CHAR_ASCII,
                                            sTempBuf,
                                            IDE_DUMP_DEST_LIMIT );

                    idlOS::memcpy( sSPName,
                                   sTempBuf + 2, // remove '; ' characters
                                   sSPNameLen );
                    sSPName[sSPNameLen] = '\0';

                    idlOS::printf("TBS NAME [Length : %"ID_UINT32_FMT"] : %s\n", sSPNameLen, sSPName);

                    break;

                case SCT_UPDATE_DRDB_CREATE_DBF:
                    idlOS::printf("## TBS Upate Type: %s < %"ID_UINT32_FMT" >,"
                                  "BEFORE - SZ: %"ID_UINT32_FMT", AFTER  - SZ: %"ID_UINT32_FMT"\n",
                                  smrLogFileDump::getTBSUptType( sTBSUptLog.mTBSUptType ),
                                  sTBSUptLog.mTBSUptType,
                                  sTBSUptLog.mBImgSize, 
                                  sTBSUptLog.mAImgSize );

                    ideLog::ideMemToHexStr( (UChar *)aLogPtr + SMR_LOGREC_SIZE(smrTBSUptLog) + sTBSUptLog.mBImgSize,
                                            sTBSUptLog.mAImgSize,
                                            IDE_DUMP_FORMAT_BINARY,
                                            sTempBuf,
                                            IDE_DUMP_DEST_LIMIT );
                    idlOS::printf("VALUE: 0x%s\n", sTempBuf);

                    sTempUInt = 0;
                    idlOS::memcpy( (void *)&sTempUInt,
                                   (UChar *)aLogPtr + SMR_LOGREC_SIZE(smrTBSUptLog) +
                                   sTBSUptLog.mBImgSize + ID_SIZEOF(smiTouchMode) +
                                   ID_SIZEOF(smiNodeAttrType) +
                                   ID_SIZEOF(scSpaceID) + SMI_MAX_DATAFILE_NAME_LEN + 2, // padding 2
                                   ID_SIZEOF(smFileID) );
                    idlOS::printf("DBFile ID : %"ID_UINT32_FMT"\n", sTempUInt);

                    idlOS::memcpy( (void *)&sSPNameLen,
                                   (UChar *)aLogPtr + SMR_LOGREC_SIZE(smrTBSUptLog) +
                                   sTBSUptLog.mBImgSize +
                                   ID_SIZEOF(smiTouchMode) +
                                   ID_SIZEOF(smiNodeAttrType) +
                                   ID_SIZEOF(scSpaceID) + SMI_MAX_DATAFILE_NAME_LEN + 2, // padding 2
                                   ID_SIZEOF(UInt) );

                    ideLog::ideMemToHexStr( (UChar *)aLogPtr +
                                            SMR_LOGREC_SIZE(smrTBSUptLog) +
                                            sTBSUptLog.mBImgSize +
                                            ID_SIZEOF(smiTouchMode) +
                                            ID_SIZEOF(smiNodeAttrType) +
                                            ID_SIZEOF(scSpaceID),
                                            sSPNameLen,
                                            IDE_DUMP_FORMAT_CHAR_ASCII,
                                            sTempBuf,
                                            IDE_DUMP_DEST_LIMIT );

                    idlOS::memcpy( sSPName,
                                   sTempBuf + 2, // remove '; ' characters
                                   sSPNameLen );
                    sSPName[sSPNameLen] = '\0';

                    idlOS::printf("DBF PATH [Length : %"ID_UINT32_FMT"] : %s\n", sSPNameLen, sSPName);

                    break;

                case SCT_UPDATE_DRDB_EXTEND_DBF:
                    idlOS::memcpy(&slBeforeValue, sLogBuffer, ID_SIZEOF(ULong));

                    idlOS::memcpy(&slAfterValue1,
                                  sLogBuffer + ID_SIZEOF(ULong),
                                  ID_SIZEOF(ULong));

                    idlOS::memcpy(&slAfterValue2,
                                  sLogBuffer + (ID_SIZEOF(ULong) * 2),
                                  ID_SIZEOF(ULong));

                    idlOS::printf("## TBS Upate Type: %s < %"ID_UINT32_FMT" >,"
                                  "BEFORE - SZ: %"ID_UINT32_FMT", VALUE:  %"ID_UINT64_FMT", " 
                                  "AFTER  - SZ: %"ID_UINT32_FMT", VALUE:  %"ID_UINT64_FMT", %"ID_UINT64_FMT"\n",
                                  smrLogFileDump::getTBSUptType( sTBSUptLog.mTBSUptType ),
                                  sTBSUptLog.mTBSUptType,
                                  sTBSUptLog.mBImgSize,
                                  slBeforeValue,
                                  sTBSUptLog.mAImgSize,
                                  slAfterValue1,
                                  slAfterValue2 );
                    break;

                default:
                    idlOS::printf("## TBS Upate Type: %s < %"ID_UINT32_FMT" >,"
                                  "BEFORE - SZ: %"ID_UINT32_FMT", AFTER  - SZ: %"ID_UINT32_FMT"\n",
                                  smrLogFileDump::getTBSUptType( sTBSUptLog.mTBSUptType ),
                                  sTBSUptLog.mTBSUptType,
                                  sTBSUptLog.mBImgSize, 
                                  sTBSUptLog.mAImgSize );
                    break;
            }
            break;

        // BUG-28266 DUMPLF으로 SAVEPOINT NAME을 볼 수 있으면 좋겠습니다
        case SMR_LT_SAVEPOINT_ABORT:
        case SMR_LT_SAVEPOINT_SET:
            sLogBuffer = ( (SChar*)aLogPtr ) + ID_SIZEOF(smrLogHead);

            idlOS::memcpy( (void *)&sSPNameLen, sLogBuffer, ID_SIZEOF(UInt) );
            sLogBuffer += ID_SIZEOF(UInt);

            idlOS::memcpy( sSPName, sLogBuffer, sSPNameLen );
            sSPName[sSPNameLen] = '\0';

            idlOS::printf( "SVP_NAME: %s", sSPName );
            break;

        case SMR_LT_XA_PREPARE:
            idlOS::memcpy( &sPrepareLog,  aLogPtr, ID_SIZEOF( smrXaPrepareLog ) );

            (void)idaXaConvertXIDToString(NULL, &sPrepareLog.mXaTransID, sXidString, SMR_XID_DATA_MAX_LEN);
            idlOS::printf("XID: %s, ", sXidString );
            idlOS::printf("LockCount: %"ID_UINT32_FMT", ", sPrepareLog.mLockCount );
            idlOS::printf("GCTX: %"ID_UINT32_FMT"", sPrepareLog.mIsGCTx );

            sCommitTIME = (time_t)sPrepareLog.mPreparedTime.tv_sec;
            idlOS::localtime_r(&sCommitTIME, &sCommit);
            idlOS::printf(" [ %4"ID_UINT32_FMT
                          "-%02"ID_UINT32_FMT
                          "-%02"ID_UINT32_FMT
                          ":%02"ID_UINT32_FMT
                          ":%02"ID_UINT32_FMT
                          ":%02"ID_UINT32_FMT" ]",
                          sCommit.tm_year + 1900,
                          sCommit.tm_mon + 1,
                          sCommit.tm_mday,
                          sCommit.tm_hour,
                          sCommit.tm_min,
                          sCommit.tm_sec);
            break;

        case SMR_LT_XA_SEGS:
            idlOS::memcpy( &sSegLog, aLogPtr, ID_SIZEOF( smrXaSegsLog ) );

            (void)idaXaConvertXIDToString(NULL, &sSegLog.mXaTransID, sXidString, SMR_XID_DATA_MAX_LEN);
            idlOS::printf("XID: %s", sXidString );
            break;

        case SMR_LT_XA_PREPARE_REQ:
            idlOS::memcpy( &sPrepareReqLog,  aLogPtr, ID_SIZEOF( smrXaPrepareReqLog ) );

            idlOS::printf("GlobalTID: %"ID_UINT32_FMT"\n", sPrepareReqLog.mGlobalTxId );

            (void)smrLogFileDump::dumpPrepareReqBranchTx( aLogPtr + SMR_LOGREC_SIZE( smrXaPrepareReqLog ),
                                                          sPrepareReqLog.mBranchTxSize );
            break;

        case SMR_LT_XA_END:
            idlOS::memcpy( &sEndLog,  aLogPtr, ID_SIZEOF( smrXaEndLog ) );
            idlOS::printf("GlobalTID: %"ID_UINT32_FMT"", sEndLog.mGlobalTxId );
            break;

        case SMR_LT_XA_START_REQ:
            idlOS::memcpy( &sStartReqLog, aLogPtr, ID_SIZEOF( smrXaStartReqLog ) );
            (void)idaXaConvertXIDToString(NULL, &sStartReqLog.mGlobalXID, sXidString, SMR_XID_DATA_MAX_LEN);
            idlOS::printf("XID: %s ", sXidString );
            break;

        case SMR_LT_MEMTRANS_COMMIT:
        case SMR_LT_DSKTRANS_COMMIT:
            idlOS::memcpy( &sCommitLog,  aLogPtr, ID_SIZEOF( smrTransCommitLog ) );
            idlOS::printf("GlobalTID: %"ID_UINT32_FMT", ", sCommitLog.mGlobalTxId );
            idlOS::printf("CommitSCN: %"ID_UINT64_FMT"", SM_SCN_TO_LONG( sCommitLog.mCommitSCN ) );
            sCommitTIME = (time_t)sCommitLog.mTxCommitTV;

            idlOS::localtime_r(&sCommitTIME, &sCommit);
            idlOS::printf(" [ %4"ID_UINT32_FMT
                          "-%02"ID_UINT32_FMT
                          "-%02"ID_UINT32_FMT
                          ":%02"ID_UINT32_FMT
                          ":%02"ID_UINT32_FMT
                          ":%02"ID_UINT32_FMT" ]\n",
                          sCommit.tm_year + 1900,
                          sCommit.tm_mon + 1,
                          sCommit.tm_mday,
                          sCommit.tm_hour,
                          sCommit.tm_min,
                          sCommit.tm_sec);
            break;

        case SMR_LT_MEMTRANS_GROUPCOMMIT:
            idlOS::memcpy( &sGCLog,  aLogPtr, ID_SIZEOF( smrTransGroupCommitLog ) );
            sCommitTIME = (time_t)sGCLog.mTxCommitTV;
            sGCCnt      = sGCLog.mGroupCnt;

            sLogBuffer = aLogPtr + SMR_LOGREC_SIZE( smrTransGroupCommitLog );

            sLogSize = SMR_LOGREC_SIZE(smrTransGroupCommitLog) + 
                       ( sGCCnt * ID_SIZEOF(smTID) * 2 ) + 
                       ID_SIZEOF(smrLogTail);
        
            if ( smrLogHeadI::getSize(&sGCLog.mHead) > sLogSize )
            {
                 /* CommitSCN 을 기록했으면 당연히 smSCN 크기의 배수 */
                IDE_DASSERT( ( ( smrLogHeadI::getSize(&sGCLog.mHead) - sLogSize ) % ID_SIZEOF(smSCN) ) == 0 );

                sGCCommitSCNBuffer = sLogBuffer + ( sGCCnt * ID_SIZEOF(smTID) * 2 );

                sIsWithCommitSCN = ID_TRUE;
            }
            else
            {
                sIsWithCommitSCN = ID_FALSE;
            }

            idlOS::localtime_r(&sCommitTIME, &sCommit);
            idlOS::printf(" [ %4"ID_UINT32_FMT
                          "-%02"ID_UINT32_FMT
                          "-%02"ID_UINT32_FMT
                          ":%02"ID_UINT32_FMT
                          ":%02"ID_UINT32_FMT
                          ":%02"ID_UINT32_FMT" ]\n",
                          sCommit.tm_year + 1900,
                          sCommit.tm_mon + 1,
                          sCommit.tm_mday,
                          sCommit.tm_hour,
                          sCommit.tm_min,
                          sCommit.tm_sec);
            idlOS::printf( "Group Count: %"ID_UINT32_FMT"\n", sGCCnt );


            for( UInt i = 0 ; i < sGCCnt ; i++ )
            {
                idlOS::memcpy( &sTID,
                               sLogBuffer,
                               ID_SIZEOF(smTID) );
                sLogBuffer += ID_SIZEOF(smTID);

                idlOS::memcpy( &sGlobalTID,
                               sLogBuffer,
                               ID_SIZEOF(smTID) );
                sLogBuffer += ID_SIZEOF(smTID);

                if ( sIsWithCommitSCN == ID_FALSE )
                {
                    idlOS::printf( "[ %"ID_UINT32_FMT" ] TID: %"ID_UINT32_FMT
                                   ", GlobalTID: %"ID_UINT32_FMT"\n",
                                   i, sTID, sGlobalTID );
                }
                else 
                {
                    idlOS::memcpy( &sCommitSCN,
                                   sGCCommitSCNBuffer,
                                   ID_SIZEOF(smSCN) );
                    sGCCommitSCNBuffer += ID_SIZEOF(smSCN);

                    idlOS::printf( "[ %"ID_UINT32_FMT" ] "
                                   "TID: %"ID_UINT32_FMT", "
                                   "GlobalTID: %"ID_UINT32_FMT", "
                                   "CommitSCN: %"ID_UINT64_FMT"\n",
                                   i, 
                                   sTID, 
                                   sGlobalTID,
                                   SM_SCN_TO_LONG( sCommitSCN ) );
                }
            } //for 
            break;

        case SMR_LT_MEMTRANS_ABORT:
        case SMR_LT_DSKTRANS_ABORT:
            break;

        case SMR_LT_UPDATE:

            idlOS::memcpy( &sUpdateLog, aLogPtr, ID_SIZEOF( smrUpdateLog ) );

            idlOS::printf(" UTYPE: %s< %"ID_UINT32_FMT" >, UPOS( SPACEID: %"ID_UINT32_FMT", PID: "
                          "%"ID_UINT32_FMT", OFFSET: %"ID_UINT32_FMT" => OID: %"ID_vULONG_FMT" ) ",
                          smrLogFileDump::getUpdateType( sUpdateLog.mType ),
                          sUpdateLog.mType,
                          SC_MAKE_SPACE( sUpdateLog.mGRID ),
                          SC_MAKE_PID( sUpdateLog.mGRID ),
                          SC_MAKE_OFFSET( sUpdateLog.mGRID ),
                          SM_MAKE_OID(SC_MAKE_PID( sUpdateLog.mGRID ),
                                      SC_MAKE_OFFSET( sUpdateLog.mGRID )));

            switch(sUpdateLog.mType)
            {
                case SMR_SMM_MEMBASE_SET_SYSTEM_SCN:
                    idlOS::memcpy(&sSCN,
                                  (SChar*)aLogPtr + SMR_LOGREC_SIZE(smrUpdateLog),
                                  ID_SIZEOF(smSCN));

                    idlOS::printf("SCN: %"ID_UINT64_FMT"", SM_SCN_TO_LONG( sSCN ) );
                    break;

                case SMR_SMC_PERS_INSERT_ROW:
                case SMR_SMC_PERS_UPDATE_VERSION_ROW:
                case SMR_SMC_PERS_UPDATE_INPLACE_ROW:
                case SMR_SMC_PERS_DELETE_VERSION_ROW:
                case SMR_SMC_SET_CREATE_SCN:
                    idlOS::printf("TableOID: %"ID_vULONG_FMT"", sUpdateLog.mData);

                    break;
                case SMR_SMM_PERS_UPDATE_NEXT_FREE_PAGE_LINK :
                    idlOS::memcpy(& sBeforePID,
                                  (SChar*)aLogPtr + SMR_LOGREC_SIZE(smrUpdateLog),
                                  ID_SIZEOF(scPageID));
                    idlOS::memcpy(& sAfterPID,
                                  (SChar*)aLogPtr + SMR_LOGREC_SIZE(smrUpdateLog) +
                                  ID_SIZEOF(scPageID),
                                  ID_SIZEOF(scPageID));
                    idlOS::printf("FLISlot: %"ID_vULONG_FMT" => %"ID_vULONG_FMT" ",
                                  sBeforePID, sAfterPID );

                    break;

                case SMR_SMM_PERS_UPDATE_LINK :
                    idlOS::memcpy(& sBeforePID,
                                  (SChar*)aLogPtr + SMR_LOGREC_SIZE(smrUpdateLog),
                                  ID_SIZEOF(scPageID));

                    idlOS::memcpy(& sAfterPID,
                                  (SChar*)aLogPtr + SMR_LOGREC_SIZE(smrUpdateLog) +
                                  ID_SIZEOF(scPageID) * 2,
                                  ID_SIZEOF(scPageID));

                    idlOS::printf("PrevPID: %"ID_vULONG_FMT" => %"ID_vULONG_FMT"",
                                  sBeforePID, sAfterPID );

                    idlOS::memcpy(& sBeforePID,
                                  (SChar*)aLogPtr + SMR_LOGREC_SIZE(smrUpdateLog) +
                                  ID_SIZEOF(scPageID) * 1,
                                  ID_SIZEOF(scPageID));

                    idlOS::memcpy(& sAfterPID,
                                  (SChar*)aLogPtr + SMR_LOGREC_SIZE(smrUpdateLog) +
                                  ID_SIZEOF(scPageID) * 3,
                                  ID_SIZEOF(scPageID));

                    idlOS::printf("NextPID: %"ID_vULONG_FMT" => %"ID_vULONG_FMT"",
                                  sBeforePID, sAfterPID );
                    break;

                default:
                    break;
            }


            /* BUG-30882 dumplf does not display the value of MRDB log. */
            idlOS::printf("\n# BEFORE - SZ: %"ID_UINT32_FMT,
                          sUpdateLog.mBImgSize );
            if( ( gDisplayValue == ID_TRUE ) &&
                ( sUpdateLog.mBImgSize > 0 ) )
            {
                ideLog::ideMemToHexStr( (UChar*)aLogPtr 
                                        + SMR_LOGREC_SIZE(smrUpdateLog),
                                        sUpdateLog.mBImgSize,
                                        IDE_DUMP_FORMAT_BINARY,
                                        sTempBuf,
                                        IDE_DUMP_DEST_LIMIT );
                idlOS::printf(", VALUE: 0x%s", sTempBuf);
            }

            idlOS::printf("\n# AFTER  - SZ: %"ID_UINT32_FMT"",
                          sUpdateLog.mAImgSize );
            if( ( gDisplayValue == ID_TRUE ) &&
                ( sUpdateLog.mAImgSize > 0 ) )
            {
                ideLog::ideMemToHexStr( (UChar*)aLogPtr +
                                        SMR_LOGREC_SIZE(smrUpdateLog) +
                                        sUpdateLog.mBImgSize,
                                        sUpdateLog.mAImgSize,
                                        IDE_DUMP_FORMAT_BINARY,
                                        sTempBuf,
                                        IDE_DUMP_DEST_LIMIT );
                idlOS::printf(", VALUE: 0x%s\n", sTempBuf);
            }

            //BUG-46854
            if ( sUpdateLog.mType == SMR_SMC_PERS_DELETE_VERSION_ROW )
            {
                sLogPtr = (UChar*)aLogPtr + SMR_LOGREC_SIZE(smrUpdateLog) 
                              + sUpdateLog.mBImgSize + sUpdateLog.mAImgSize;

                // 프라이머리 키 로깅 했을 때 
                if( ( smrLogHeadI::getFlag( aLogHead ) & SMR_LOG_RP_INFO_LOG_MASK )
                    == SMR_LOG_RP_INFO_LOG_OK )
                {
                    idlOS::memcpy(&sPrimaryKeySize, sLogPtr, ID_SIZEOF(UInt));
                    sLogPtr += sPrimaryKeySize;
                    
                    idlOS::printf("# PrimaryKeySize: %"ID_UINT32_FMT"\n", sPrimaryKeySize);
                }

                // Full XLog 일 때 
                if( ( smrLogHeadI::getFlag( aLogHead ) & SMR_LOG_FULL_XLOG_MASK )
                    == SMR_LOG_FULL_XLOG_OK )
                {
                    idlOS::memcpy(&sFullXLogSize, sLogPtr, ID_SIZEOF(UInt));
                    sLogPtr += sFullXLogSize;

                    idlOS::printf("# FullXLogSize: %"ID_UINT32_FMT"\n", sFullXLogSize);
                }
                
                idlOS::memcpy(&sColCnt, sLogPtr, ID_SIZEOF(UInt));
                idlOS::printf("# Column Cnt: %"ID_UINT32_FMT"\n", sColCnt);
                sLogPtr += ID_SIZEOF(UInt);

                for ( UInt i = 0 ; i < sColCnt ; i++ )
                {               
                    idlOS::memcpy(&sPieceCnt, sLogPtr, ID_SIZEOF(UInt));
                    idlOS::printf("# Piece Cnt: %"ID_UINT32_FMT"\n", sPieceCnt);
                    sLogPtr += ID_SIZEOF(UInt);

                    for ( UInt j = 0 ; j < sPieceCnt ; j++ )
                    {
                        idlOS::memcpy(&sVCPieceOID, sLogPtr, ID_SIZEOF(smOID));
                        idlOS::printf("# VarColPieceOID: %"ID_vULONG_FMT", ", sVCPieceOID);
                        sLogPtr += ID_SIZEOF(smOID);

                        idlOS::memcpy(&sBefFlag, sLogPtr, ID_SIZEOF(UShort));
                        idlOS::printf("# before VALUE: 0x%x, ", sBefFlag);
                        sLogPtr += ID_SIZEOF(UShort);

                        idlOS::memcpy(&sAftFlag, sLogPtr, ID_SIZEOF(UShort));
                        idlOS::printf("# after VALUE: 0x%x\n", sAftFlag);
                        sLogPtr += ID_SIZEOF(UShort);

                        idlOS::printf("# PID: %u, Offset: %u\n", 
                                       SM_MAKE_PID( sVCPieceOID ),
                                       SM_MAKE_OFFSET( sVCPieceOID ) ); 
                    }
                }
            }
            break;

        case SMR_LT_CHKPT_BEGIN:
            idlOS::memcpy( &sBeginChkptLog, aLogPtr, ID_SIZEOF( smrBeginChkptLog ) );

            idlOS::printf("MemRecovery LSN ( %"ID_UINT32_FMT", %"ID_UINT32_FMT" ) ",
                          sBeginChkptLog.mEndLSN.mFileNo,
                          sBeginChkptLog.mEndLSN.mOffset );
            idlOS::printf("DiskRecovery LSN ( %"ID_UINT32_FMT", %"ID_UINT32_FMT" ) ",
                          sBeginChkptLog.mDiskRedoLSN.mFileNo,
                          sBeginChkptLog.mDiskRedoLSN.mOffset );
            idlOS::printf("\n");
            break;

         case SMR_LT_CHKPT_END:
             break;

         case SMR_LT_COMPENSATION:
            idlOS::memcpy( &sCMPLog, aLogPtr, ID_SIZEOF( smrCMPSLog ) );

            sLogBuffer = ((SChar*)aLogPtr + SMR_LOGREC_SIZE(smrCMPSLog));

            if (sCMPLog.mTBSUptType != SCT_UPDATE_MAXMAX_TYPE)
            {
                idlOS::printf("SPACEID: %"ID_UINT32_FMT"\n",
                                      SC_MAKE_SPACE(sCMPLog.mGRID));

                if ( (sctUpdateType)sCMPLog.mTBSUptType
                                    == SCT_UPDATE_DRDB_EXTEND_DBF)
                {
                    idlOS::memcpy(&slBeforeValue,
                                  sLogBuffer,
                                  ID_SIZEOF(ULong));

                    idlOS::printf("## TBS Upate Type: %s< %"ID_UINT32_FMT" >, BEFORE - SZ: %"ID_UINT32_FMT", VALUE: %"ID_UINT64_FMT"\n",
                                  smrLogFileDump::getTBSUptType( sCMPLog.mTBSUptType ),
                                  sCMPLog.mTBSUptType,
                                  sCMPLog.mBImgSize,
                                  slBeforeValue );
                }
                else
                {
                    idlOS::printf("## TBS Upate Type: %s< %"ID_UINT32_FMT" >, BEFORE - SZ: %"ID_UINT32_FMT"\n",
                                  smrLogFileDump::getTBSUptType( sCMPLog.mTBSUptType ),
                                  sCMPLog.mTBSUptType,
                                  sCMPLog.mBImgSize);
                }
            }
            else
            {
                idlOS::printf("Undo Log Info:Type: %s< %"ID_UINT32_FMT" >, SPACEID: %"ID_UINT32_FMT", PID: %"ID_UINT32_FMT", "
                              "OFFSET: %"ID_vULONG_FMT", BEFORE - SZ: %"ID_UINT32_FMT", VALUE: %"ID_vULONG_FMT"\n",
                              smrLogFileDump::getUpdateType( sCMPLog.mUpdateType ),
                              sCMPLog.mUpdateType,
                              SC_MAKE_SPACE( sCMPLog.mGRID ),
                              SC_MAKE_PID( sCMPLog.mGRID ),
                              SC_MAKE_OFFSET( sCMPLog.mGRID ),
                              sCMPLog.mBImgSize,
                              sCMPLog.mData);
            }

            /* BUG-30882 dumplf does not display the value of MRDB log. */
            ideLog::ideMemToHexStr( (UChar*)aLogPtr
                                    + SMR_LOGREC_SIZE(smrCMPSLog),
                                    sCMPLog.mBImgSize,
                                    IDE_DUMP_FORMAT_BINARY,
                                    sTempBuf,
                                    IDE_DUMP_DEST_LIMIT );
            idlOS::printf("# BEFORE - SZ: %"ID_UINT32_FMT,
                          sCMPLog.mBImgSize );
            
            if( ( gDisplayValue == ID_TRUE ) &&
                ( sCMPLog.mBImgSize > 0 ) )
            {
                idlOS::printf(", VALUE: 0x%s\n", sTempBuf);
            }

            //BUG-46854: delete version row의 플래그 출력 
            if ( sCMPLog.mUpdateType == SMR_SMC_PERS_DELETE_VERSION_ROW )
            {
                sLogPtr = (UChar*)aLogPtr + SMR_LOGREC_SIZE(smrCMPSLog)  
                          + sCMPLog.mBImgSize;

                idlOS::memcpy(&sColCnt, sLogPtr, ID_SIZEOF(UInt));
                idlOS::printf("# Column Cnt: %"ID_UINT32_FMT"\n", sColCnt);
                sLogPtr += ID_SIZEOF(UInt);

                for ( UInt i = 0 ; i < sColCnt ; i++ )
                {               
                    idlOS::memcpy(&sPieceCnt, sLogPtr, ID_SIZEOF(UInt));
                    idlOS::printf("# Piece Cnt: %"ID_UINT32_FMT"\n", sPieceCnt);
                    sLogPtr += ID_SIZEOF(UInt);

                    for ( UInt j = 0 ; j < sPieceCnt ; j++ )
                    {
                        idlOS::memcpy(&sVCPieceOID, sLogPtr, ID_SIZEOF(smOID));
                        idlOS::printf("# VarColPieceOID: %"ID_vULONG_FMT", ", sVCPieceOID);
                        sLogPtr += ID_SIZEOF(smOID);

                        idlOS::memcpy(&sBefFlag, sLogPtr, ID_SIZEOF(UShort));
                        idlOS::printf("# before VALUE: 0x%x, ", sBefFlag);
                        sLogPtr += ID_SIZEOF(UShort);

                        idlOS::printf("# PID: %u, Offset: %u\n", 
                                      SM_MAKE_PID( sVCPieceOID ),
                                      SM_MAKE_OFFSET( sVCPieceOID ) ); 
                    }
                }
            }

            break;

        case SMR_DLT_COMPENSATION:
            idlOS::memcpy( &sDiskCMPLog, aLogPtr, ID_SIZEOF( smrDiskCMPSLog ) );

            sLogBuffer = ((SChar*)aLogPtr + SMR_LOGREC_SIZE(smrDiskCMPSLog));

            idlOS::printf("Undo Log Info:Type:SMR_DLT_COMPENSATION\n");

            dumpDiskLog( sLogBuffer,
                         sDiskCMPLog.mRedoLogSize,
                         NULL );
            break;

        case SMR_LT_NTA:
            idlOS::memcpy( &sNTALog, aLogPtr, ID_SIZEOF( smrNTALog ) );
            idlOS::printf("OPTYPE: %s< %"ID_UINT32_FMT" >, Data1: %"ID_UINT64_FMT", Data2: %"ID_UINT64_FMT"\n",
                          smrLogFileDump::getOPType( sNTALog.mOPType ),
                          sNTALog.mOPType,
                          sNTALog.mData1,
                          sNTALog.mData2);

            break;

        case SMR_LT_FILE_BEGIN:
            idlOS::memcpy( &sFileBeginLog ,aLogPtr, ID_SIZEOF( smrFileBeginLog ) );
            idlOS::printf("FileNo: %"ID_UINT32_FMT"\n",
                          sFileBeginLog.mFileNo );

            break;

        case SMR_LT_FILE_END:
            break;

        /* For LOB: BUG-15946 */
        case SMR_LT_LOB_FOR_REPL:
            idlOS::memcpy( &sLobLog, aLogPtr, ID_SIZEOF( smrLobLog ) );

            idlOS::printf( "LOB Locator: %"ID_UINT64_FMT", OPType: %s< %"ID_UINT32_FMT" >\n",
                           sLobLog.mLocator,
                           smrLogFileDump::getLOBOPType( sLobLog.mOpType ),
                           sLobLog.mOpType );

            break;

        case SMR_SMM_MEMBASE_SET_SYSTEM_SCN:
            break;

        case SMR_LT_DDL :           // DDL Transaction임을 표시하는 Log Record
            break;

        case SMR_LT_TABLE_META :    // Table Meta Log Record
            idlOS::memcpy( &sTableMetaLog, aLogPtr, ID_SIZEOF( smrTableMetaLog ) );

            idlOS::printf("Old Table OID: %"ID_vULONG_FMT", "
                          "New Table OID: %"ID_vULONG_FMT", "
                          "User Name: %s, Table Name: %s, Partition Name: %s\n",
                          sTableMetaLog.mTableMeta.mOldTableOID,
                          sTableMetaLog.mTableMeta.mTableOID,
                          sTableMetaLog.mTableMeta.mUserName,
                          sTableMetaLog.mTableMeta.mTableName,
                          sTableMetaLog.mTableMeta.mPartName);
            break;


        case SMR_LT_DDL_QUERY_STRING: // PROJ-1723
            aLogPtr += ID_SIZEOF(smrLogHead);
            idlOS::memcpy( &sDDLStmtMeta, aLogPtr, ID_SIZEOF( smrDDLStmtMeta ) );

            idlOS::printf("OBJ: %s.%s ",
                          sDDLStmtMeta.mUserName, sDDLStmtMeta.mTableName);

            aLogPtr += ID_SIZEOF(smrDDLStmtMeta);
            aLogPtr += ID_SIZEOF(SInt);

            idlOS::printf("SQL: %s\n", aLogPtr);
            break;

        default:
            break;
    }

    sState = 0;
    IDE_TEST( iduMemMgr::free( sTempBuf ) != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    switch(sState)
    {
    case 1:
        IDE_ASSERT( iduMemMgr::free( sTempBuf ) == IDE_SUCCESS );
    default:
        break;
    }

    return IDE_FAILURE;
}


/***********************************************************************
 * Description : dump disk log buffer
 *
 * TASK-4007 [SM] PBT를 위한 기능 추가
 * 8바이트 이상 로그들에 대해서도 Value를 출력해준다.
 **********************************************************************/
void dumpDiskLog( SChar         * aLogBuffer,
                  UInt            aAImgSize,
                  UInt          * aOPType )
{
    UInt        sParseLen;
    SChar     * sLogRec;
    sdrLogHdr   sLogHdr;
    ULong       sLValue;
    UInt        sIValue;
    UShort      sSValue;
    UChar       sCValue;
    scSpaceID   sSpaceID;
    scPageID    sPageID;
    scOffset    sOffset;
    scSlotNum   sSlotNum;
    SChar     * sTempBuf;
    UInt        sState = 0;

    IDE_DASSERT( aLogBuffer != NULL );

    /* 해당 메모리 영역을 Dump한 결과값을 저장할 버퍼를 확보합니다.
     * Stack에 선언할 경우, 이 함수를 통해 서버가 종료될 수 있으므로
     * Heap에 할당을 시도한 후, 성공하면 기록, 성공하지 않으면 그냥
     * return합니다. */
    IDE_TEST( iduMemMgr::calloc( IDU_MEM_ID, 1,
                                 ID_SIZEOF( SChar ) * IDE_DUMP_DEST_LIMIT,
                                 (void**)&sTempBuf )
              != IDE_SUCCESS );
    sState = 1;

    sParseLen = 0;

    if ( aOPType != NULL )
    {
        idlOS::printf("## NTA: %s< %"ID_UINT32_FMT" >\n", 
                      smrLogFileDump::getDiskOPType( *aOPType ),
                      *aOPType );
    }

    while (sParseLen < aAImgSize)
    {
        sLogRec = aLogBuffer + sParseLen;

        idlOS::memcpy(&sLogHdr, sLogRec, ID_SIZEOF(sdrLogHdr));

        sSpaceID = SC_MAKE_SPACE(sLogHdr.mGRID);
        sPageID  = SC_MAKE_PID(sLogHdr.mGRID);
        if( SC_GRID_IS_WITH_SLOTNUM(sLogHdr.mGRID) )
        {
            sOffset  = SC_NULL_OFFSET;
            sSlotNum = SC_MAKE_SLOTNUM(sLogHdr.mGRID);
        }
        else
        {
            sOffset  = SC_MAKE_OFFSET(sLogHdr.mGRID);
            sSlotNum = SC_NULL_SLOTNUM;
        }

        /* TASK-4007 [SM] PBT를 위한 기능 추가
         * 모든 Log의 Header를 출력해준 후 Value를 Hexa로 Dump하여 출력해준다.
         * value값 출력 여부를 결정한다.*/
        switch (sLogHdr.mType)
        {
        case SDR_SDP_1BYTE:
            idlOS::memcpy( &sCValue,
                           sLogRec + ID_SIZEOF( sdrLogHdr ), ID_SIZEOF( UChar ) );
            idlOS::printf( "# TYPE: %s < %"ID_UINT32_FMT" >, SPACEID: %"ID_UINT32_FMT", PID: %"ID_UINT32_FMT", SLOTNUM: %"ID_UINT32_FMT", OFFSET: %"ID_UINT32_FMT", SZ: %"ID_UINT32_FMT", VALUE: %"ID_UINT32_FMT" \n",
                           smrLogFileDump::getDiskLogType( sLogHdr.mType ),
                           sLogHdr.mType,
                           sSpaceID,
                           sPageID,
                           sSlotNum,
                           sOffset,
                           sLogHdr.mLength,
                           sCValue );
            break;

        case SDR_SDP_2BYTE:
            idlOS::memcpy( &sSValue,
                           sLogRec + ID_SIZEOF( sdrLogHdr ), ID_SIZEOF( UShort ) );
            idlOS::printf( "# TYPE: %s < %"ID_UINT32_FMT" >, SPACEID: %"ID_UINT32_FMT", PID: %"ID_UINT32_FMT", SLOTNUM: %"ID_UINT32_FMT", OFFSET: %"ID_UINT32_FMT", SZ: %"ID_UINT32_FMT", VALUE: %"ID_UINT32_FMT" \n",
                           smrLogFileDump::getDiskLogType( sLogHdr.mType ),
                           sLogHdr.mType,
                           sSpaceID,
                           sPageID,
                           sSlotNum,
                           sOffset,
                           sLogHdr.mLength,
                           sSValue );
            break;

        case SDR_SDP_4BYTE:
            idlOS::memcpy( &sIValue,
                           sLogRec + ID_SIZEOF(sdrLogHdr), ID_SIZEOF( UInt ) );
            idlOS::printf( "# TYPE: %s < %"ID_UINT32_FMT" >, SPACEID: %"ID_UINT32_FMT", PID: %"ID_UINT32_FMT", SLOTNUM: %"ID_UINT32_FMT", OFFSET: %"ID_UINT32_FMT", SZ: %"ID_UINT32_FMT", VALUE: %"ID_UINT32_FMT" \n",
                           smrLogFileDump::getDiskLogType( sLogHdr.mType ),
                           sLogHdr.mType,
                           sSpaceID,
                           sPageID,
                           sSlotNum,
                           sOffset,
                           sLogHdr.mLength,
                           sIValue );
            break;

        case SDR_SDP_8BYTE:
            idlOS::memcpy( &sLValue,
                           sLogRec + ID_SIZEOF( sdrLogHdr ), ID_SIZEOF( ULong ) );
            idlOS::printf("# TYPE: %s < %"ID_UINT32_FMT" >, SPACEID: %"ID_UINT32_FMT", PID: %"ID_UINT32_FMT", SLOTNUM: %"ID_UINT32_FMT", OFFSET: %"ID_UINT32_FMT", SZ: %"ID_UINT32_FMT", VALUE: %"ID_UINT32_FMT" \n",
                          smrLogFileDump::getDiskLogType( sLogHdr.mType ),
                          sLogHdr.mType,
                          sSpaceID,
                          sPageID,
                          sSlotNum,
                          sOffset,
                          sLogHdr.mLength,
                          sLValue );
            break;

        default :
            idlOS::printf("# TYPE: %s < %"ID_UINT32_FMT" >, SPACEID: %"ID_UINT32_FMT", PID: %"ID_UINT32_FMT", SLOTNUM: %"ID_UINT32_FMT", OFFSET: %"ID_UINT32_FMT", SZ: %"ID_UINT32_FMT,
                          smrLogFileDump::getDiskLogType( sLogHdr.mType ),
                          sLogHdr.mType,
                          sSpaceID,
                          sPageID,
                          sSlotNum,
                          sOffset,
                          sLogHdr.mLength);

            /* TASK-4007 [SM]PBT를 위한 기능 추가
             * 8바이트 초과 Log의 value도 Dump하여 보여준다*/
            if( ( gDisplayValue == ID_TRUE ) &&
                ( sLogHdr.mLength > 0 ) )
            {
                ideLog::ideMemToHexStr( (UChar*)(sLogRec + ID_SIZEOF(sdrLogHdr)),
                                        sLogHdr.mLength,
                                        IDE_DUMP_FORMAT_BINARY,
                                        sTempBuf,
                                        IDE_DUMP_DEST_LIMIT );
        
                idlOS::printf( ", VALUE: 0x%s\n", sTempBuf );
            }
            else
            {
                idlOS::printf( "\n" );
            }
            break;
        }

        sParseLen += (ID_SIZEOF(sdrLogHdr) + sLogHdr.mLength);
    }

    if (sParseLen != aAImgSize)
    {
        idlOS::printf("parsing length : %"ID_UINT32_FMT", after length : %"ID_UINT32_FMT" !!    invalid log size\n",
                      sParseLen, aAImgSize);
    }

    sState = 0;
    IDE_TEST( iduMemMgr::free( sTempBuf ) != IDE_SUCCESS );

    return;

    IDE_EXCEPTION_END;

    switch(sState)
    {
    case 1:
        IDE_ASSERT( iduMemMgr::free( sTempBuf ) == IDE_SUCCESS );
    default:
        break;
    }

    return;
}



/* TASK-4007 [SM] PBT를 위한 기능 추가
 * Log를 Dump할때, 어떤 로그를 어떻게 출력할지를 서버가 아닌 Dump하고자 하는
 * Util에서 결정하도록, 읽어드린 로그에 대한 처리용 Callback함수를 둔다.*/
/* Callback 구조는 확장성이 떨어지기 때문에, Class구조로 변경한다. */
IDE_RC dumpLogFile()
{
    smrLogType       sLogType;
    idBool           sEOF;
    smrLogHead     * sLogHead;
    SChar          * sLogPtr;
    UInt             sOffset;
    idBool           sIsComp   = ID_FALSE;
    idBool           sIsOpened = ID_FALSE;

    IDE_TEST( gLogFileDump.openFile( gLogFileName ) != IDE_SUCCESS );
    sIsOpened = ID_TRUE;

    for(;;)
    {
        sEOF = ID_FALSE;
        IDE_TEST( gLogFileDump.dumpLog( &sEOF ) != IDE_SUCCESS );

        sLogHead = gLogFileDump.getLogHead();

        if( smrLogHeadI::getType(sLogHead) == SMR_LT_FILE_END )
        {
            sOffset  = gLogFileDump.getOffset();

            IDE_TEST( dumpLogHead( sOffset,
                                   sLogHead, 
                                   sIsComp )
                      != IDE_SUCCESS );

            idlOS::printf( "\n\n" );
        }

        if( sEOF == ID_TRUE )
        {
            break;
        }

        sLogPtr  = gLogFileDump.getLogPtr();
        sOffset  = gLogFileDump.getOffset();
        sIsComp  = gLogFileDump.getIsCompressed();

        /* TID 조건에 따라 출력 여부 결정 */
        if( ( gTID == 0 ) || 
            ( smrLogHeadI::getTransID( sLogHead ) == gTID ) ) 
        {
            idlOS::memcpy( &sLogType,
                           sLogPtr + smrLogHeadI::getSize( sLogHead ) - ID_SIZEOF( smrLogTail ),
                           ID_SIZEOF( smrLogType ) );

            IDE_TEST( dumpLogHead( sOffset,
                                   sLogHead, sIsComp )
                      != IDE_SUCCESS );

            IDE_TEST( dumpLogBody( smrLogHeadI::getType( sLogHead ),
                                   sLogHead,  
                                   (SChar*) sLogPtr )
                      != IDE_SUCCESS );

            if( smrLogHeadI::getType( sLogHead ) != sLogType )
            {
                idlOS::printf( "    Invalid Log!!! Head %"ID_UINT32_FMT
                               " <> Tail %"ID_UINT32_FMT"\n",
                               smrLogHeadI::getType( sLogHead ),
                               sLogType );

                idlOS::printf("----------------------------------------"
                              "----------------------------------------\n" );
                IDE_TEST( dumpLogBody( sLogType, sLogHead, (SChar*) sLogPtr )
                        != IDE_SUCCESS );
                idlOS::printf("----------------------------------------"
                              "----------------------------------------\n" );
            }


            idlOS::printf( "\n\n" );
            idlOS::fflush( NULL );
        }
    }

    sIsOpened = ID_FALSE;
    IDE_TEST( gLogFileDump.closeFile() != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    if( sIsOpened == ID_TRUE )
    {
        (void)gLogFileDump.closeFile();
    }

    return IDE_FAILURE;
}


/***********************************************************************
 * display log types only
 **********************************************************************/
void displayLogTypes()
{
    UInt    i;
    
    // Log Type 출력
    idlOS::printf( "LogType:\n" );

    // BUG-28581 dumplf에서 Log 개수 계산을 위한 sizeof 단위가 잘못되어 있습니다.
    // LogType배열에 있는 모든 내용을 토해냅니다.
    for( i=0; i<ID_UCHAR_MAX ; i++ )
    {
        idlOS::printf( "%3"ID_UINT32_FMT" %s\n", 
                       i, 
                       smrLogFileDump::getLogType( i ) );
    }
    idlOS::printf( "\n" );

    // Log Operation Type 출력
    idlOS::printf( "LogOPType:\n" );
    for( i=0; i<SMR_OP_MAX+1; i++ )
    {
        idlOS::printf( "%3"ID_UINT32_FMT" %s\n", 
                       i, 
                       smrLogFileDump::getOPType( i ) );
    }
    idlOS::printf( "\n" );

    // Lob Op Type 출력
    idlOS::printf( "LobOPType:\n" );
    for( i=0; i<SMR_LOB_OP_MAX+1; i++ )
    {
        idlOS::printf( "%3"ID_UINT32_FMT" %s\n", 
                       i, 
                       smrLogFileDump::getLOBOPType( i ) );
    }
    idlOS::printf( "\n" );

    // Disk OP Type 출력
    idlOS::printf( "DiskOPType:\n" );
    for( i=0; i<SDR_OP_MAX+1; i++)
    {
        idlOS::printf( "%3"ID_UINT32_FMT" %s\n", 
                       i, 
                       smrLogFileDump::getDiskOPType( i ) );
    }
    idlOS::printf( "\n" );

    // Disk Log Type 출력
    idlOS::printf( "Disk LogType:\n" );
    for( i=0; i<SDR_LOG_TYPE_MAX+1; i++)
    {
        idlOS::printf( "%3"ID_UINT32_FMT" %s\n", 
                       i, 
                       smrLogFileDump::getDiskLogType( i ) );
    }
    idlOS::printf( "\n" );

    // Update Type 출력
    idlOS::printf( "UpdateType:\n" );
    for( i=0; i<SM_MAX_RECFUNCMAP_SIZE; i++)
    {
        idlOS::printf( "%3"ID_UINT32_FMT" %s\n", 
                       i, 
                       smrLogFileDump::getUpdateType( i ) );
    }
    idlOS::printf( "\n" );

    // TBS Upt Type 출력
    idlOS::printf( "TBSUptType:\n" );
    for( i=0; i<SCT_UPDATE_MAXMAX_TYPE+1; i++)
    {
        idlOS::printf( "%3"ID_UINT32_FMT" %s\n", 
                       i, 
                       smrLogFileDump::getTBSUptType( i ) );
    }
    idlOS::printf( "\n" );
}


/***********************************************************************
 * Hashtable에 등록된 모든 TableOIDInfo 를 출력한다.
 * 각각의 Hash Element에 대해 호출될 Visitor함수
 **********************************************************************/
IDE_RC displayingVisitor( vULong   aKey,
                          void   * aData,
                          void   * /*aVisitorArg*/ )
{
    smOID         sTableOID  = (smOID)aKey;
    gTableInfo  * sTableInfo = (gTableInfo *) aData;
    
    if( sTableInfo != NULL )
    {
        IDE_ERROR( sTableInfo->mTableOID == sTableOID );

        gInserLogCnt4Debug  += sTableInfo->mInserLogCnt;
        gUpdateLogCnt4Debug += sTableInfo->mUpdateLogCnt;
        gDeleteLogCnt4Debug += sTableInfo->mDeleteLogCnt;
        gHashItemCnt4Debug  ++;

        idlOS::printf( "-------------------------\n"
                       " [ TABLEOID : %"ID_UINT64_FMT" ]\n"
                       "-------------------------\n",
                       sTableInfo->mTableOID );

        idlOS::printf( " INSERT = %"ID_UINT64_FMT "\n"
                       " UPDATE = %"ID_UINT64_FMT "\n"
                       " DELETE = %"ID_UINT64_FMT "\n\n",
                        sTableInfo->mInserLogCnt,
                        sTableInfo->mUpdateLogCnt,
                        sTableInfo->mDeleteLogCnt );
    } 
    else
    {
        /* nothing to do */
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}


/***********************************************************************
 * Hashtable에 등록된 모든 TableOIDInfo 를 destroy한다.
 * 각각의 Hash Element에 대해 호출될 Visitor함수
 **********************************************************************/
IDE_RC destoyingVisitor( vULong   aKey,
                         void   * aData,
                         void   * /*aVisitorArg*/ )
{
    smOID         sTableOID  = (smOID)aKey;
    gTableInfo  * sTableInfo = (gTableInfo *) aData;

    if( sTableInfo != NULL )
    {
        IDE_ERROR( sTableInfo->mTableOID == sTableOID );

        IDE_TEST( iduMemMgr::free( sTableInfo ) != IDE_SUCCESS );
        IDE_TEST( gTableInfoHash.remove( sTableOID ) != IDE_SUCCESS );
    } 
    else
    {
        /* nothing to do */
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/***********************************************************************
 *  Log의 Body의 타입별 count 증가
 * 
 *  aLogType [IN] 출력할 로그의 Type
 *  aLogBody [IN] 출력할 로그의 Body
 **********************************************************************/
IDE_RC dumpLogBodyStatisticsGroupbyTableOID( UInt aLogType, SChar *aLogPtr )
{
    smrUpdateLog    sUpdateLog;
    gTableInfo    * sTableInfo = NULL;
    smOID           sTableOID  = SM_NULL_OID;
    UInt            sState = 0;

    /* BUG-47525 Group Commit */
    smrTransGroupCommitLog sGCLog;

    switch(aLogType)
    {
        case SMR_LT_MEMTRANS_COMMIT:
            gCommitLogCnt++;
            break;
        case SMR_LT_MEMTRANS_GROUPCOMMIT:
            idlOS::memcpy( &sGCLog, aLogPtr, ID_SIZEOF( smrTransGroupCommitLog ) );
            gCommitLogCnt += sGCLog.mGroupCnt;
            break;
        case SMR_LT_MEMTRANS_ABORT:
            gAbortLogCnt++;
            break;
        case SMR_LT_UPDATE:

            idlOS::memcpy( &sUpdateLog, aLogPtr, ID_SIZEOF( smrUpdateLog ) );

            sTableOID = sUpdateLog.mData; 
           
            IDE_TEST_CONT( SM_IS_VALID_OID(sTableOID) != ID_TRUE, SKIP );
   
            // sTableOID가 hash table에 있는지 확인
            sTableInfo = (gTableInfo *)gTableInfoHash.search( sTableOID );
            //  sTableOID가 처음 나온것이라면 hash table에 삽입
            if( sTableInfo == NULL )
            {
                IDE_TEST_RAISE( iduMemMgr::calloc(IDU_MEM_SM_SMU,
                                                  1,
                                                  ID_SIZEOF(gTableInfo),
                                                  (void**) &sTableInfo ) 
                                != IDE_SUCCESS, insufficient_memory );
                sState = 1;
            
                sTableInfo->mTableOID = sTableOID;
                IDE_TEST( gTableInfoHash.insert( sTableOID, sTableInfo ) != IDE_SUCCESS );
                gHashItemCnt++;

                // sTableOID가 hash table에 있는지 확인
                // 방금 삽입했으니 없으면 문제가 있음
                sTableInfo = (gTableInfo *)gTableInfoHash.search( sTableOID );
                IDE_ERROR( sTableInfo != NULL );
            }
            else
            {
                /* nothing to do */
            }
         
            IDE_ERROR( sTableInfo->mTableOID == sTableOID );

            switch(sUpdateLog.mType)
            {
                case SMR_SMC_PERS_INSERT_ROW:
                    sTableInfo->mInserLogCnt++;
                    gInserLogCnt++;
                    break;
                case SMR_SMC_PERS_UPDATE_VERSION_ROW:
                case SMR_SMC_PERS_UPDATE_INPLACE_ROW:
                    sTableInfo->mUpdateLogCnt++;
                    gUpdateLogCnt++;
                    break;
                case SMR_SMC_PERS_DELETE_VERSION_ROW:
                    sTableInfo->mDeleteLogCnt++;
                    gDeleteLogCnt++;
                    break;
                default:
                    break;
            }
            break;
        default:
            break;
    }

    IDE_EXCEPTION_CONT( SKIP );

    return IDE_SUCCESS;

    IDE_EXCEPTION( insufficient_memory );
    {
        IDE_SET(ideSetErrorCode(idERR_ABORT_InsufficientMemory));
    }
    IDE_EXCEPTION_END;

    switch( sState )
    {
        case 1: 
            IDE_ASSERT( iduMemMgr::free( sTableInfo ) == IDE_SUCCESS );
        default:
            break;
    }
    return IDE_FAILURE;
}

/***********************************************************************
 *  Log의 Body의 타입별 count 증가
 * 
 *  aLogType [IN] 출력할 로그의 Type
 *  aLogBody [IN] 출력할 로그의 Body
 **********************************************************************/
IDE_RC dumpLogBodyStatistics( UInt aLogType, SChar *aLogPtr )
{
    smrUpdateLog  sUpdateLog;

    /* BUG-47525 Group Commit */
    smrTransGroupCommitLog sGCLog;

    switch(aLogType)
    {
        case SMR_LT_MEMTRANS_COMMIT:
            gCommitLogCnt++;
            break;
        case SMR_LT_MEMTRANS_GROUPCOMMIT:
            idlOS::memcpy( &sGCLog, aLogPtr, ID_SIZEOF( smrTransGroupCommitLog ) );
            gCommitLogCnt += sGCLog.mGroupCnt;
            break;
        case SMR_LT_MEMTRANS_ABORT:
            gAbortLogCnt++;
            break;
        case SMR_LT_UPDATE:

            idlOS::memcpy( &sUpdateLog, aLogPtr, ID_SIZEOF( smrUpdateLog ) );

            switch(sUpdateLog.mType)
            {
                case SMR_SMC_PERS_INSERT_ROW:
                    gInserLogCnt++;
                    break;
                case SMR_SMC_PERS_UPDATE_VERSION_ROW:
                case SMR_SMC_PERS_UPDATE_INPLACE_ROW:
                    gUpdateLogCnt++;
                    break;
                case SMR_SMC_PERS_DELETE_VERSION_ROW:
                    gDeleteLogCnt++;
                    break;
                default:
                    break;
            }
            break;
        default:
            break;
    }

    return IDE_SUCCESS;
}

/******************************************************************
 * aLogFile    [IN]  분석할 파일의 절대경로
 *****************************************************************/
IDE_RC dumpLogFileStatistics( SChar *aLogFile )
{
    smrLogType       sLogType;
    smrLogHead     * sLogHead;
    SChar          * sLogPtr;
    idBool           sEOF      = ID_FALSE;
    idBool           sIsOpened = ID_FALSE;
    smLSN            sLSN;
    // 파일 열어서
    IDE_TEST( gLogFileDump.openFile( aLogFile ) != IDE_SUCCESS );
    sIsOpened = ID_TRUE;

    // 빈 파일이면 skip
    sEOF = ID_FALSE;
    IDE_TEST( gLogFileDump.dumpLog( &sEOF ) != IDE_SUCCESS ); 

    if( sEOF == ID_TRUE ) 
    {
        IDE_CONT(SKIP);
    }

    sLogHead = gLogFileDump.getLogHead();

    if( smrLogHeadI::getType(sLogHead) != SMR_LT_FILE_BEGIN ) 
    {
        /* 파일의 처음이므로 SMR_LT_FILE_BEGIN 이어야 함.
           아니면 altibase logfile이 아닌것으로 판단함 */
        gInvalidFile++;
        idlOS::printf( "Warning : %s Invalid Log File. skip this file.\n", 
                       aLogFile );
        IDE_CONT(SKIP);
    }

    for(;;)
    {
        // 로그하나 읽어서
        sEOF = ID_FALSE;
        IDE_TEST( gLogFileDump.dumpLog( &sEOF ) != IDE_SUCCESS ); 

        if( sEOF == ID_TRUE ) 
        {
            break;
        }
        // 읽어야 하는 로그 정보를 가져온다.
        sLogHead = gLogFileDump.getLogHead();
        sLogPtr  = gLogFileDump.getLogPtr();

        sLSN = smrLogHeadI::getLSN( sLogHead );
        /* SN 조건에 따라 수집 여부 결정 */
        /* BUG-44361               sLSN >= gLSN */
        if ( smrCompareLSN::isGTE( &sLSN, &gLSN ) )
        {
            idlOS::memcpy( &sLogType,
                    sLogPtr + smrLogHeadI::getSize( sLogHead ) - ID_SIZEOF( smrLogTail ),
                    ID_SIZEOF( smrLogType ) );

            if( gGroupByTableOID == ID_TRUE )
            {
                IDE_TEST( dumpLogBodyStatisticsGroupbyTableOID( smrLogHeadI::getType( sLogHead ),
                                                                (SChar*) sLogPtr )
                          != IDE_SUCCESS );
            }
            else
            {
                IDE_TEST( dumpLogBodyStatistics( smrLogHeadI::getType( sLogHead ),
                                                 (SChar*) sLogPtr )
                          != IDE_SUCCESS );
            }
            if( smrLogHeadI::getType( sLogHead ) != sLogType )
            {
                gInvalidLog++;
            }
            else
            {
                /* nothing to do */
            }
        }
    }
    IDE_EXCEPTION_CONT( SKIP );

    sIsOpened = ID_FALSE;
    IDE_TEST( gLogFileDump.closeFile() != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    if( sIsOpened == ID_TRUE )
    {
        (void)gLogFileDump.closeFile();
    }
    return IDE_FAILURE;
}

/******************************************************************
// atoi의 long형 버전
 *****************************************************************/
ULong atol(SChar *aSrc)
{
    ULong   sRet = 0;

    while ((*aSrc) != '\0')
    {
        sRet = sRet * 10;
        if ( ( (*aSrc) < '0' ) || ( '9' < (*aSrc) ) )
        {
            break;
        }
        else
        {
            // nothing to do...
        }

        sRet += (ULong)( (*aSrc) - '0');
        aSrc ++;
    }

    return sRet;
}

/******************************************************************
 * 로그 파일에서 숫자만 비교해서 큰 순으로 나열하는 용도
 *****************************************************************/
extern "C" SInt
compareFD( const void* aElem1, const void* aElem2 )
{
    const fdEntry *sLhs = (const fdEntry*) aElem1;
    const fdEntry *sRhs = (const fdEntry*) aElem2;
    SChar sLhsName[SM_MAX_FILE_NAME+1] = {"", };
    SChar sRhsName[SM_MAX_FILE_NAME+1] = {"", };
    UInt  sFileIdx1 = ID_UINT_MAX;
    UInt  sFileIdx2 = ID_UINT_MAX;
    UInt  sOffset   = idlOS::strlen(SMR_LOG_FILE_NAME); 

    idlOS::strcpy( sLhsName, (const char*)sLhs->sName+sOffset );
    idlOS::strcpy( sRhsName, (const char*)sRhs->sName+sOffset );
    sFileIdx1 = idlOS::atoi( sLhsName );
    sFileIdx2 = idlOS::atoi( sRhsName );

    if ( sFileIdx1 < sFileIdx2 )
    {
        return 1;
    }
    else
    {
        if ( sFileIdx1 > sFileIdx2 )
        {
            return -1;
        }
        else
        {
            /* nothing to do */
        }
    }

    return 0;
}

/******************************************************************
 * 지정된 SN을 포함하는 파일을 찾는다.
 *****************************************************************/
IDE_RC findFirstFile( fdEntry  *aEntries, UInt *aFileNum )
{
    SChar sFullFileName[SM_MAX_FILE_NAME+1] = {"", };
    smrLogHead     * sLogHead;
    idBool           sEOF      = ID_FALSE;
    idBool           sIsOpened = ID_FALSE;
    UInt             i = 0;
    smLSN            sLSN;

    IDE_TEST( gFileIdx < 1 );
    *aFileNum = gFileIdx-1;  // 못찾으면 처음부터 해야 하니까.

    for ( i = 0 ; i < gFileIdx ; i++ )
    {
        if( idlOS::strlen(aEntries[i].sName) < idlOS::strlen(SMR_LOG_FILE_NAME) )
        {
            idlOS::printf( "Warning : %s invalid log file name.\n", 
                           aEntries[i].sName );
            IDE_TEST(1); 
        }
        else
        {
            /* nothing to do */
        }
        idlOS::snprintf( sFullFileName, SM_MAX_FILE_NAME, "%s%c%s",
                         gPath, IDL_FILE_SEPARATOR, aEntries[i].sName );

        // 파일 열어서
        IDE_TEST( gLogFileDump.openFile( sFullFileName ) != IDE_SUCCESS );
        sIsOpened = ID_TRUE;

        sEOF = ID_FALSE;
        IDE_TEST( gLogFileDump.dumpLog( &sEOF ) != IDE_SUCCESS ); 

        // 빈 파일이면 skip
        if( sEOF == ID_TRUE ) 
        {
            sIsOpened=ID_FALSE;
            IDE_TEST( gLogFileDump.closeFile() != IDE_SUCCESS );
            continue;
        }
        else
        {
            /* nothing to do */
        }

        sLogHead = gLogFileDump.getLogHead();

        if ( smrLogHeadI::getType(sLogHead) != SMR_LT_FILE_BEGIN ) 
        {
            sIsOpened=ID_FALSE;
            IDE_TEST( gLogFileDump.closeFile() != IDE_SUCCESS );
            continue;
        }

        // SN이 작은 파일이 찾아 지면 이파일 부터 분석을 시작하면 된다. */
        sLSN = smrLogHeadI::getLSN( sLogHead );
        /* BUG-44361               gLSN >= sLSN */
        if ( smrCompareLSN::isGTE( &gLSN, &sLSN ) )
        {
            *aFileNum = i;
            break;
        } 

        sIsOpened=ID_FALSE;
        IDE_TEST( gLogFileDump.closeFile() != IDE_SUCCESS );

    } //for

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    if( sIsOpened == ID_TRUE )
    {
        (void)gLogFileDump.closeFile();
    }
    return IDE_FAILURE;
} 


/******************************************************************
 * scan 할 파일 목록을 작성한다.
 *****************************************************************/
IDE_RC makeTargetFileList( fdEntry  ** aEntries )
{
    DIR              * sDIR       = NULL;
    FILE             * sFP        = NULL;
    struct  dirent   * sDirEnt    = NULL;
    struct  dirent   * sResDirEnt = NULL;
    SChar              sFullFileName[SM_MAX_FILE_NAME+1] = {"", };
    SChar              sFileName[SM_MAX_FILE_NAME+1]     = {"", };
    SInt               sRC;
    ULong              sAllocSize = 512;

    IDE_TEST_RAISE( iduMemMgr::calloc( IDU_MEM_SM_SMU,
                                       1,
                                       ID_SIZEOF(struct dirent) + SM_MAX_FILE_NAME,
                                       (void**)&(sDirEnt) )
                    != IDE_SUCCESS, insufficient_memory );

    sDIR = idf::opendir( gPath );
    IDE_TEST_RAISE(sDIR == NULL, err_open_dir);

    errno = 0;
    sRC = idf::readdir_r(sDIR, sDirEnt, &sResDirEnt) ;
    IDE_TEST_RAISE( (sRC != 0) && (errno != 0), err_read_dir );

    IDE_TEST_RAISE( iduMemMgr::calloc( IDU_MEM_SM_SMU,
                                       1,
                                       ID_SIZEOF(fdEntry) * sAllocSize,
                                       (void**)aEntries )
                    != IDE_SUCCESS, insufficient_memory );

    gFileIdx  = 0;
    while( sResDirEnt != NULL )
    {
        idlOS::strcpy( sFileName, (const char*)sResDirEnt->d_name );
        if ((idlOS::strcmp(sFileName, ".") == 0 ) || (idlOS::strcmp(sFileName, "..") == 0) )
        {
            /*
             * BUG-31529  [sm] errno must be initialized before calling readdir_r.
             */
            errno = 0;
            sRC = idf::readdir_r(sDIR, sDirEnt, &sResDirEnt);
            IDE_TEST_RAISE( (sRC != 0) && (errno != 0), err_read_dir );
            errno = 0;

            continue;
        }
        else
        {
            /* nothing to do */
        }

        //logfilexx 로 시작하는 파일을 찾아서
        if(idlOS::strncmp(sFileName, SMR_LOG_FILE_NAME, idlOS::strlen(SMR_LOG_FILE_NAME) ) == 0)
        {
            idlOS::snprintf( sFullFileName, SM_MAX_FILE_NAME, "%s%c%s",
                             gPath, IDL_FILE_SEPARATOR, sFileName );
             
            //파일을 열어보고
            sFP = idlOS::fopen( sFullFileName, "r");
            if(sFP == NULL)
            {
                gInvalidFile++;
                idlOS::printf( "Warning : %s cannot be opened. skip this file.\n", 
                               sFullFileName );
            }   
            else
            {
                // 목록을 만들어서
                idlOS::strcpy((*aEntries)[gFileIdx].sName, (const char*)sResDirEnt->d_name);

                if( idlOS::strncmp( (*aEntries)[gFileIdx].sName, sFileName, idlOS::strlen(sFileName) ) != 0 )
                {
                    
                    gInvalidFile++;
                    idlOS::printf( "Warning : %s invalid log file name.  skip this file.\n", 
                                   (*aEntries)[gFileIdx].sName );
                    
                    idlOS::fclose( sFP );

                    errno = 0;
                    sRC = idf::readdir_r(sDIR, sDirEnt, &sResDirEnt);
                    IDE_TEST_RAISE( (sRC != 0) && (errno != 0), err_read_dir );
                    errno = 0;

                    continue; 
                }
                gFileIdx++;

                if ( gFileIdx >= sAllocSize )
                {
                    // fdEntry 가 부족하면 sAllocSize씩 추가
                    sAllocSize = (gFileIdx + sAllocSize);
                    IDE_TEST_RAISE( iduMemMgr::realloc( IDU_MEM_SM_SMU,
                                                        ID_SIZEOF(fdEntry) * sAllocSize,
                                                        (void**)aEntries )
                            != IDE_SUCCESS, insufficient_memory);
                }
                else
                {
                    /* nothing to do */
                }
            }
            idlOS::fclose( sFP );
        }
        else
        {
            /* nothing to do */
        }

        errno = 0;
        sRC = idf::readdir_r(sDIR, sDirEnt, &sResDirEnt);
        IDE_TEST_RAISE( (sRC != 0) && (errno != 0), err_read_dir );
        errno = 0;
    }//while

    if( sDIR != NULL )
    {
        idf::closedir(sDIR);
        sDIR = NULL;
    }
    if( sDirEnt != NULL )
    {
        IDE_TEST( iduMemMgr::free(sDirEnt) != IDE_SUCCESS );
        sDirEnt = NULL;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION(err_open_dir);
    {
        IDE_SET( ideSetErrorCode( smERR_FATAL_CannotOpenDir, gPath ) );
    }
    IDE_EXCEPTION(err_read_dir);
    {
        IDE_SET( ideSetErrorCode( smERR_FATAL_CannotReadDir, gPath ) );
    }
    IDE_EXCEPTION( insufficient_memory );
    {
        IDE_SET(ideSetErrorCode(idERR_ABORT_InsufficientMemory));
    }
    IDE_EXCEPTION_END;

    if( sDIR != NULL )
    {
        idf::closedir(sDIR);
        sDIR = NULL;
    }
    if( sDirEnt != NULL )
    {
        (void)iduMemMgr::free(sDirEnt);
        sDirEnt = NULL;
    }
    return IDE_FAILURE;

}
/******************************************************************
 * 지정된 경로/파일 의 통계정보 (DML의 대략적인 수) 를 확인한다.
 *****************************************************************/
IDE_RC dumpStatistics()
{
    SChar       sFullFileName[SM_MAX_FILE_NAME+1] = {"", };
    fdEntry   * sEntries = NULL;
    UInt        sFileIdx = 0; // 지정된 SN을 포함하는 파일인덱스
    SLong       i;

    IDE_ERROR ( SM_IS_LSN_MAX( gLSN ) != ID_TRUE );

    // -g : TableOID로 Grouping 하기 위해 hash table 생성
    if( gGroupByTableOID == ID_TRUE )
    {
        IDE_TEST( gTableInfoHash.initialize(IDU_MEM_SM_SMU,
                    TABLEOID_HASHTABLE_SIZE-1, /* aInitFreeBucketCount */ 
                    TABLEOID_HASHTABLE_SIZE  /* aHashTableSize*/)
                != IDE_SUCCESS);
    }
    else
    {
        /* nothing to do */
    }

    // -f : specify log file name  
    if( gLogFileName != NULL )
    {
        // 특정 파일 하나만 분석을 함.
        gFileIdx = 1;
        IDE_TEST( dumpLogFileStatistics( gLogFileName )
                  != IDE_SUCCESS );
    }
    else // -F : specify the path of logs
    {
        // scan 할 파일 목록 작성
        IDE_TEST( makeTargetFileList(&sEntries) != IDE_SUCCESS );

        // 번호가 큰순으로 파일들 정렬해서
        idlOS::qsort( (void*)sEntries, gFileIdx, ID_SIZEOF(fdEntry), compareFD );
      
        // 특정 SN 이 지정이 되었다면 해당 SN을 포함하는 파일을 구한다.
        if ( SM_IS_LSN_INIT( gLSN ) != ID_TRUE )
        { 
            IDE_TEST( findFirstFile( sEntries, &sFileIdx ) != IDE_SUCCESS );
        }
        else
        {
            //특정 SN 이 지정이 되지 않으면 처음 파일부터 끝까지 scan
            if( gFileIdx != 0 )
            {
                sFileIdx = gFileIdx-1;
            }
        }

        // gFileIdx이 0이면 scan 할 로그 파일이 없습니다.
        if( gFileIdx != 0 )
        { 
            // SN을 포함하는 파일부터 scan 시작
            for ( i = sFileIdx ; i >= 0 ; i-- )
            {
                if( idlOS::strlen(sEntries[i].sName) < idlOS::strlen(SMR_LOG_FILE_NAME) )
                {
                    idlOS::printf( "Warning : %s invalid log file name.\n", 
                                   sEntries[i].sName );
                    IDE_TEST(1); 
                }

                idlOS::snprintf( sFullFileName, SM_MAX_FILE_NAME, "%s%c%s",
                                 gPath, IDL_FILE_SEPARATOR, sEntries[i].sName );
                IDE_TEST( dumpLogFileStatistics( sFullFileName ) 
                          != IDE_SUCCESS );
            } //for 
        }
        else
        {
            /* nothing to do */
        }
    }//else

    // -g : TableOID로 Grouping 해서 출력
    if( gGroupByTableOID == ID_TRUE )
    {
        // hash Table을 순회 하면서 내용을 출력한다.
        IDE_TEST( gTableInfoHash.traverse( displayingVisitor,
                                           NULL /* Visitor Arg */ )
                  != IDE_SUCCESS );
        // 총합이 맞는지 한번 확인.
        IDE_TEST_RAISE( gInserLogCnt4Debug != gInserLogCnt, err_invalid_cnt );
        IDE_TEST_RAISE( gUpdateLogCnt4Debug != gUpdateLogCnt, err_invalid_cnt);
        IDE_TEST_RAISE( gDeleteLogCnt4Debug != gDeleteLogCnt, err_invalid_cnt);
        IDE_TEST_RAISE( gHashItemCnt4Debug  != gHashItemCnt, err_invalid_cnt );
    }

    idlOS::printf( "----------------------------------------\n"
                   " [ MMDB Statistics ] \n"
                   "----------------------------------------\n" );

    idlOS::printf( " INSERT = %"ID_UINT64_FMT "\n"
                   " UPDATE = %"ID_UINT64_FMT "\n"
                   " DELETE = %"ID_UINT64_FMT "\n"
                   " COMMIT = %"ID_UINT64_FMT "\n"
                   " ROLLBACK = %"ID_UINT64_FMT "\n",
                   gInserLogCnt,
                   gUpdateLogCnt,
                   gDeleteLogCnt,
                   gCommitLogCnt,
                   gAbortLogCnt );
#if defined(DEBUG)
    idlOS::printf( " #TableCnt : %"ID_UINT64_FMT", "
                   " #LogFile : %"ID_UINT64_FMT", "
                   " #InvalidLog : %"ID_UINT64_FMT", "
                   " #InvalidFile : %"ID_UINT64_FMT"\n",
                   gHashItemCnt,
                   gFileIdx,
                   gInvalidLog,
                   gInvalidFile );  

    if( (SM_IS_LSN_INIT( gLSN ) != ID_TRUE) && (gLogFileName == NULL) )
    {
        idlOS::printf( " SN %"ID_UINT32_FMT",%"ID_UINT32_FMT" in"
                       " %s.\n",
                       gLSN.mFileNo,
                       gLSN.mOffset,
                       sEntries[sFileIdx].sName );  
    }    
#endif

    if( sEntries != NULL )
    {
        IDE_TEST(iduMemMgr::free( sEntries )  != IDE_SUCCESS );
        sEntries = NULL;
    }

    if( gGroupByTableOID == ID_TRUE )
    {
        // hash 및 TableInfo 객체 정리한다.
        IDE_TEST( gTableInfoHash.traverse( destoyingVisitor,
                                           NULL /* Visitor Arg */ )
              != IDE_SUCCESS );
        
        IDE_TEST( gTableInfoHash.destroy() != IDE_SUCCESS );
    }        

    return IDE_SUCCESS;
    
    IDE_EXCEPTION( err_invalid_cnt );
    {
        idlOS::printf( "Warning : They have the wrong values between the log count and sum of TableOID.\n" );
        idlOS::printf( "log count : %"ID_UINT64_FMT " \n", gHashItemCnt );
        idlOS::printf( " INSERT = %"ID_UINT64_FMT "\n"
                       " UPDATE = %"ID_UINT64_FMT "\n"
                       " DELETE = %"ID_UINT64_FMT "\n",
                       gInserLogCnt,
                       gUpdateLogCnt,
                       gDeleteLogCnt );
        idlOS::printf( "sum of TableOID : %"ID_UINT64_FMT "\n", gHashItemCnt4Debug );
        idlOS::printf( " INSERT = %"ID_UINT64_FMT "\n"
                       " UPDATE = %"ID_UINT64_FMT "\n"
                       " DELETE = %"ID_UINT64_FMT "\n",
                       gInserLogCnt4Debug,
                       gUpdateLogCnt4Debug,
                       gDeleteLogCnt4Debug );


    }
    IDE_EXCEPTION_END;

    if( sEntries != NULL )
    {
        (void)iduMemMgr::free( sEntries );
        sEntries = NULL;
    }

    return IDE_FAILURE;
}


int main( int aArgc, char *aArgv[] )
{
    SChar*  sDir;
    idBool  sIsInitialized = ID_FALSE;

    // BUG-44361
    SM_LSN_MAX(gLSN);

    parseArgs( aArgc, aArgv );

    // BUG-44361
    IDE_TEST_RAISE( parseLSN( &gLSN, gStrLSN ) != IDE_SUCCESS,
                    err_invalid_arguments )

    if( gDisplayHeader == ID_TRUE)
    {
        showCopyRight();
    }

    IDE_TEST_RAISE( gInvalidArgs == ID_TRUE,
                    err_invalid_arguments );

    IDE_TEST_RAISE( gLogFileName == NULL && 
                    gDisplayLogTypes == ID_FALSE &&
                    SM_IS_LSN_MAX( gLSN ),
                    err_invalid_arguments );

    if( gLogFileName != NULL )
    {
        IDE_TEST_RAISE( (idlOS::access(gLogFileName, F_OK) != 0) && 
                        (gDisplayLogTypes == ID_FALSE),
                        err_not_exist_file );
    }
    else
    {
        if( SM_IS_LSN_MAX( gLSN ) != ID_TRUE )
        {   
            // 특정 경로가 지정되지 않았다면 프로퍼티 파일을 읽어서 경로를 설정.
            if(gPathSet == ID_FALSE)
            {
                IDE_TEST_RAISE( idp::initialize(NULL, NULL) != IDE_SUCCESS, 
                                err_load_property );
                IDE_TEST_RAISE( iduProperty::load() != IDE_SUCCESS, 
                                err_load_property );
                IDE_TEST_RAISE( idp::readPtr( "LOG_DIR", (void **)&sDir) != IDE_SUCCESS, 
                                err_load_property );
                idlOS::strcpy( gPath, sDir );
            }
            else
            {
                /* nothing do do */
            }
            // 설정된 경로가 정상인지 확인
            IDE_TEST_RAISE( idf::opendir(gPath) == NULL,
                            err_not_exist_path )
        }
        else
        {
            // -f / -S / -l 중 하나는 지정이 되어야 한다.
            IDE_TEST_RAISE( gDisplayLogTypes == ID_FALSE,
                            err_not_specified_file );
        }
    }

    IDE_TEST( initProperty() != IDE_SUCCESS );

    /* Global Memory Manager initialize */
    IDE_TEST( iduMemMgr::initializeStatic( IDU_CLIENT_TYPE ) != IDE_SUCCESS );
    IDE_TEST( iduMutexMgr::initializeStatic( IDU_CLIENT_TYPE ) != IDE_SUCCESS );

    //fix TASK-3870
    (void)iduLatch::initializeStatic(IDU_CLIENT_TYPE);

    IDE_TEST( iduCond::initializeStatic() != IDE_SUCCESS);

    IDE_TEST_RAISE( smrLogFileDump::initializeStatic() != IDE_SUCCESS,
                    err_logfile_dump_init );

    // display log types only
    if( gDisplayLogTypes == ID_TRUE )
    {
        displayLogTypes();
    }
    else
    {
        IDE_TEST( gLogFileDump.initFile() != IDE_SUCCESS );
        sIsInitialized  = ID_TRUE;

        //display DML statistic
        if( SM_IS_LSN_MAX( gLSN ) != ID_TRUE )
        {
            IDE_TEST_RAISE( dumpStatistics() != IDE_SUCCESS,
                            err_statistic_dump );
        }
        else
        {   // dump specific log file
            IDE_TEST_RAISE( dumpLogFile() != IDE_SUCCESS,
                            err_logfile_dump);
        }

        sIsInitialized = ID_FALSE;
        IDE_TEST( gLogFileDump.destroyFile() != IDE_SUCCESS );
    }

    IDE_TEST_RAISE( smrLogFileDump::destroyStatic() != IDE_SUCCESS,
                    err_logfile_dump_fini );

    idlOS::printf( "\nDump complete.\n" );

    
    if( (gLogFileName == NULL)
        && (SM_IS_LSN_MAX( gLSN ) != ID_TRUE)
        && (gPathSet == ID_FALSE) )
    {
        IDE_ASSERT( idp::destroy() == IDE_SUCCESS );
    }
    else
    {
        /* nothing to do */
    }

    IDE_ASSERT( iduCond::destroyStatic() == IDE_SUCCESS);
    (void)iduLatch::destroyStatic();
    IDE_ASSERT( iduMutexMgr::destroyStatic() == IDE_SUCCESS );
    IDE_ASSERT( iduMemMgr::destroyStatic() == IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION( err_logfile_dump_init );
    {
        idlOS::printf( "\nFailed to initialize Dump Module.\n" );
    }
    IDE_EXCEPTION( err_logfile_dump_fini );
    {
        idlOS::printf( "\nFailed to destroy Dump Module.\n" );
    }
    IDE_EXCEPTION( err_logfile_dump );
    {
        idlOS::printf( "\nDump doesn't completed.\n" );
    }
    IDE_EXCEPTION( err_statistic_dump );
    {
        idlOS::printf( "\nDump statistic doesn't completed.\n" );
    }
    IDE_EXCEPTION( err_load_property );
    {
        idlOS::printf( "\nFailed to loading LOG_DIR.\n" );
    }
    IDE_EXCEPTION( err_not_exist_path );
    {
        idlOS::printf( "\nThe path doesn't exist.\n" );
    }
    IDE_EXCEPTION( err_not_exist_file );
    {
        idlOS::printf( "\nThe logfile doesn't exist.\n" );
    }
    IDE_EXCEPTION( err_not_specified_file );
    {
        idlOS::printf( "\nPlease specify a log file name.\n" );

        (void)usage();
    }
    IDE_EXCEPTION( err_invalid_arguments );
    {
        (void)usage();
    }
    IDE_EXCEPTION_END;

    if( sIsInitialized == ID_TRUE )
    {    
        (void)gLogFileDump.destroyFile();
    }

    return IDE_FAILURE;
}

void showCopyRight( void )
{
    SChar         sBuf[1024 + 1];
    SChar         sBannerFile[1024];
    SInt          sCount;
    FILE        * sFP;
    SChar       * sAltiHome;
    const SChar * sBanner = "DUMPLF.ban";

    sAltiHome = idlOS::getenv( IDP_HOME_ENV );
    IDE_TEST_RAISE( sAltiHome == NULL, err_altibase_home );

    // make full path banner file name
    idlOS::memset(   sBannerFile, 0, ID_SIZEOF( sBannerFile ) );
    idlOS::snprintf( sBannerFile, 
                     ID_SIZEOF( sBannerFile ), 
                     "%s%c%s%c%s",
                     sAltiHome, 
                     IDL_FILE_SEPARATOR, 
                     "msg",
                     IDL_FILE_SEPARATOR, 
                     sBanner );

    sFP = idlOS::fopen( sBannerFile, "r" );
    IDE_TEST_RAISE( sFP == NULL, err_file_open );

    sCount = idlOS::fread( (void*) sBuf, 1, 1024, sFP );
    IDE_TEST_RAISE( sCount <= 0, err_file_read );

    sBuf[sCount] = '\0';
    idlOS::printf( "%s", sBuf );
    idlOS::fflush( stdout );

    idlOS::fclose( sFP );

    IDE_EXCEPTION( err_altibase_home );
    {
        // nothing to do
        // ignore error in ShowCopyright
    }
    IDE_EXCEPTION( err_file_open );
    {
        // nothing to do
        // ignore error in ShowCopyright
    }
    IDE_EXCEPTION( err_file_read );
    {
        idlOS::fclose( sFP );
    }
    IDE_EXCEPTION_END;
}

IDE_RC parseLSN( smLSN * aLSN, const char * aStrLSN )
{
    /* aStrLSN example) 1,1000 */
    smLSN            sLsn;
    const SChar    * sPointCheck;
    const SChar    * sPointOffset;

    IDE_ERROR( aLSN != NULL );

    IDE_TEST_CONT( aStrLSN == NULL, SKIP_PARSE );

    sPointCheck = aStrLSN;
    while ( ( ( *sPointCheck >= '0' ) && ( *sPointCheck <= '9' ) )
            || ( *sPointCheck == ',' ) )
    {
        ++sPointCheck;
    }
    IDE_TEST_RAISE( *sPointCheck != 0,
                    err_invalid_lsn_format );
    
    sPointOffset = idlOS::strchr(aStrLSN, ',');
    IDE_TEST_RAISE( sPointOffset == NULL,
                    err_invalid_lsn_format );

    ++sPointOffset;

    sLsn.mFileNo    = idlOS::atoi( aStrLSN );
    sLsn.mOffset    = idlOS::atoi( sPointOffset );

    *aLSN = sLsn;

    IDE_EXCEPTION_CONT( SKIP_PARSE );

    return IDE_SUCCESS;

    IDE_EXCEPTION( err_invalid_lsn_format );
    {
        idlOS::printf( "\nFailed to parse LSN. (%s)\n", aStrLSN );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}
