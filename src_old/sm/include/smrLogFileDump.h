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
 * $Id: smrLogFileDump.h 22903 2007-08-08 01:53:38Z kmkim $
 **********************************************************************/

#ifndef _O_SMR_LOG_FILE_DUMP_H_
#define _O_SMR_LOG_FILE_DUMP_H_ 1

#include <ida.h>
#include <idu.h>
#include <smDef.h>

#define SMR_XID_DATA_MAX_LEN (256)

/*
    하나의 로그파일안의 로그레코드들을 출력한다.
 */
class smrLogFileDump
{
public :
    /* Static 변수 및 함수들 
     *********************************************************************/
    static IDE_RC initializeStatic();
    static IDE_RC destroyStatic();
    
    /* TASK-4007 [SM] PBT를 위한 기능 추가
     * 서버내 저장되어 있는 LogType, Operation Type 등
     * 로그와 관련된 String 정보를 가져온다. */
    static SChar *getLogType( smrLogType aIdx );
    static SChar *getOPType( UInt aIdx );
    static SChar *getLOBOPType( UInt aIdx );
    static SChar *getDiskOPType( UInt aIdx );
    static SChar *getDiskLogType( UInt aIdx );
    static SChar *getUpdateType( smrUpdateType aIdx );
    static SChar *getTBSUptType( UInt aIdx );

    static IDE_RC dumpPrepareReqBranchTx( SChar * aBranchTxStr, UInt aSize );

    /* Instance 함수들
     *********************************************************************/
    IDE_RC initFile();
    IDE_RC destroyFile();
    IDE_RC allocBuffer();
    IDE_RC freeBuffer();

    IDE_RC openFile( SChar * aStrLogFile );
    IDE_RC closeFile();
    IDE_RC dumpLog( idBool * aEOF );

    ULong        getFileSize()     { return mFileSize;}
    UInt         getFileNo()       { return mFileNo;}
    smrLogHead * getLogHead()      { return &mLogHead;}
    SChar      * getLogPtr()       { return mLogPtr;}
    UInt         getOffset()       { return mOffset;}
    UInt         getNextOffset()   { return mNextOffset;}
    idBool       getIsCompressed() { return mIsCompressed;}
    
private:
    /* Dump하는데 필요한 member 변수 */
    iduFile                 mLogFile;
    SChar                 * mFileBuffer;
    ULong                   mFileSize;
    iduReusedMemoryHandle   mDecompBuffer;

    UInt                    mFileNo;
    smrLogHead              mLogHead;
    SChar                 * mLogPtr;
    UInt                    mOffset;
    UInt                    mNextOffset;
    idBool                  mIsCompressed;

    /* 로그메모리의 특정 Offset에서 로그 레코드를 읽어온다.
     * 압축된 로그의 경우, 로그 압축해제를 수행한다. */
    static IDE_RC readLog( iduMemoryHandle    * aDecompBufferHandle,
                           SChar              * aFileBuffer,
                           UInt                 aFileNo,
                           UInt                 aLogOffset,
                           smrLogHead         * aRawLogHead,
                           SChar             ** aRawLogPtr,
                           UInt               * aLogSizeAtDisk,
                           idBool             * aIsCompressed);


    // Log File Header로부터 File Begin로그를 얻어낸다
    static IDE_RC getFileNo( SChar * aFileBeginLog,
                             UInt  * aFileNo );
    
    // BUG-28581 dumplf에서 Log 개수 계산을 위한 sizeof 단위가 잘못되어 있습니다.
    // smrLogType는 UChar이기 때문에 UChar MAX만큼 Array를 생성해야 합니다.
    static SChar mStrLogType[ ID_UCHAR_MAX ][100];
    static SChar mStrOPType[SMR_OP_MAX+1][100];
    static SChar mStrLOBOPType[SMR_LOB_OP_MAX+1][100];
    static SChar mStrDiskOPType[SDR_OP_MAX+1][100];
    static SChar mStrDiskLogType[SDR_LOG_TYPE_MAX+1][128];
    static SChar mStrUpdateType[SM_MAX_RECFUNCMAP_SIZE][100];
    static SChar mStrTBSUptType[SCT_UPDATE_MAXMAX_TYPE+1][64];
};

inline SChar *smrLogFileDump::getLogType( smrLogType aIdx )
{
    //smrLogType은 UChar형 변수로 256개 있으며, mStrLogType은 1Byte로
    //선언 되어 있기에, Overflow 문제가 없다.

    return mStrLogType[ aIdx ];
}

inline SChar *smrLogFileDump::getOPType( UInt aIdx )
{
    if( SMR_OP_MAX < aIdx )
    {
        aIdx = SMR_OP_MAX;
    }
    else
    {
        /* nothing to do ... */
    }

    return mStrOPType[ aIdx ];
}

inline SChar *smrLogFileDump::getLOBOPType( UInt aIdx )
{
    if( SMR_LOB_OP_MAX < aIdx )
    {
        aIdx = SMR_LOB_OP_MAX ;
    }
    else
    {
        /* nothing to do ... */
    }

    return mStrLOBOPType[ aIdx ];
}

inline SChar *smrLogFileDump::getDiskOPType( UInt aIdx )
{
    if( SDR_OP_MAX < aIdx )
    {
        aIdx = SDR_OP_MAX;
    }
    else
    {
        /* nothing to do ... */
    }

    return mStrDiskOPType[ aIdx ];
}

inline SChar *smrLogFileDump::getDiskLogType( UInt aIdx )
{
    if( SDR_LOG_TYPE_MAX < aIdx )
    {
        aIdx = SDR_LOG_TYPE_MAX;
    }
    else
    {
        /* nothing to do ... */
    }

    return mStrDiskLogType[ aIdx ];
}

inline SChar *smrLogFileDump::getUpdateType( smrUpdateType aIdx )
{
    if( SM_MAX_RECFUNCMAP_SIZE <= aIdx )
    {
        aIdx = SM_MAX_RECFUNCMAP_SIZE - 1;
    }
    else
    {
        /* nothing to do ... */
    }

    return mStrUpdateType[ aIdx ];
}

inline SChar *smrLogFileDump::getTBSUptType( UInt aIdx )
{
    if( SCT_UPDATE_MAXMAX_TYPE < aIdx )
    {
        aIdx = SCT_UPDATE_MAXMAX_TYPE;
    }
    return mStrTBSUptType[ aIdx ];
}

#endif /* _O_SMR_LOG_FILE_DUMP_H_ */
