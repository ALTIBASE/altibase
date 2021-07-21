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
 * $Id:
 **********************************************************************/

#ifndef _O_SMI_READ_LOG_BYORDER_H_
#define _O_SMI_READ_LOG_BYORDER_H_ 1

#include <smDef.h>
#include <smrLogFile.h>
#include <smrPreReadLFileThread.h>
#include <iduReusedMemoryHandle.h>
#include <smrRemoteLogMgr.h>

/* Parallel Logging : Log를 생성
   순서대로 보내야 하는 Replicaiton을 위해 구현되었다.
   현재 보내야 하는 Log의 위치를 가지고
   있고, 보낼때 현재 가리키는 로그의 LSN중 가장 작은 Log를
   읽어서 보낸다.
   .*/
typedef struct smiReadInfo
{
    /* Read할 Log LSN */
    smLSN       mReadLSN;
    /* mReadLSN이 가리키는 Log의 LogHead */
    smrLogHead  mLogHead;
    /* mReadLSN이 가리키는 Log의 LogBuffer Ptr - 비압축 로그*/
    SChar     * mLogPtr;
    /* mReadLSN이 가리키는 Log가 있는 logfile Ptr */
    smrLogFile* mLogFilePtr;
    /* mReadLSN이 가리키는 Log가 Valid하면 ID_TRUE, 아니면 ID_FALSE */
    idBool      mIsValid;

    /* 마지막으로 Read한 Valid 로그의 LSN값을 Return한다.*/
    smLSN        mLstLogLSN;

    /* Log Switch*/
    idBool      mIsLogSwitch;

    /* 로그의 압축해제 버퍼 핸들
     * 
     * 압축된 로그의 경우, 이 핸들이 지니는 메모리에 압축을 해제하고
     * 로그레코드의 포인터가 이 메모리를 직접 가리키게 된다.
     * 따라서, smiReadInfo하나당 압축 해제 버퍼를 지녀야 한다. */
    iduReusedMemoryHandle mDecompBufferHandle;

    /* 읽은 로그의 크기
     * 
     * 압축된 로그의 경우 로그의 크기와
     * 파일상의 크기가 다를 수 있다.
     * 다음 읽을 로그의 위치를 계산할때 이 값을 사용하여야 한다. */
    UInt mLogSizeAtDisk;
} smiReadInfo;

class smiReadLogByOrder
{
public:
    /* 초기화 수행 */
    IDE_RC initialize( smSN       aInitSN,
                       UInt       aPreOpenFileCnt,
                       idBool     aIsRemoteLog,
                       ULong      aLogFileSize, 
                       SChar   ** aLogDirPath,
                       idBool     aNotDecomp );  /*압축된 로그파일의 경우 압축을 풀지않고 반환할지 여부 (압축된 형태로 반환)*/

    /* 할당된 Resource를 정리한다.*/
    IDE_RC destroy();

    /* mSortRedoInfo에 들어있는 smrRedoInfo중에서
     * 가장 작은 mLSN값을 가진 Log를 읽어들인다. */
    IDE_RC readLog( smSN       * aInitSN,
                    smLSN      * aLSN,
                    void      ** aLogHeadPtr,
                    void      ** aLogPtr,
                    idBool     * aIsValid );

    idBool isCompressedLog( SChar * aRawLog );

    smOID  getTableOID( SChar * aRawLog );

    IDE_RC decompressLog( SChar      * aCompLog,
                          smLSN      * aReadLSN,
                          smiLogHdr  * aLogHead,
                          SChar     ** aLogPtr );

    /* 모든 LogFile을 조사해서 aMinLSN보다 작은 LSN을
       가지는 로그를 첫번째로 가지는 LogFile No를 구해서 첫번째로 읽을로그로
       Setting한다.*/
    IDE_RC setFirstReadLogFile( smLSN aInitLSN );
    /* setFirstReadLogFile에서 찾은 파일에 대해서 실제로 첫번째로 읽어야할
       로그의 위치를 찾는다. 즉 aInitLSN보다 크거나 같은 LSN값을 가진 로그중
       가장 작은 LSN값을 가진 log의 위치를 찾는다.*/
    IDE_RC setFirstReadLogPos( smLSN aInitLSN );

    /* valid한 로그를 읽어들인다.*/
    IDE_RC searchValidLog( idBool *aIsValid );
    
    /* PROJ-1915 
      * 오프라인 로그에서 마지막 으로 기록된 LSN을 얻는다. 
      */
    IDE_RC getRemoteLastUsedGSN( smSN * aSN );

    /*
      현재 읽고 있는 지점이전의 모든 로그가
      Sync가 되었는지 Check한다.
    */
    IDE_RC isAllReadLogSynced( idBool *aIsSynced );
  
    IDE_RC stop();
    IDE_RC startByLSN(smLSN aLstReadLSN);
    /*
      Pre Fetch할 로그파일 개수를 재지정한다 
    */ 
    void    setPreReadFileCnt(UInt aFileCnt)   { mPreReadFileCnt = aFileCnt; }

    /*
      Read 정보를 제공한다.
    */
    void   getReadLSN( smLSN *aReadLSN );

    smiReadLogByOrder(){};
    virtual ~smiReadLogByOrder(){};

    iduMemoryHandle * getDecompBufferHandle() { return &( mReadInfo.mDecompBufferHandle ); }
    UInt              getDecompOffset() { return mReadInfo.mReadLSN.mOffset; }
    UInt              getFileNo() { return mReadInfo.mReadLSN.mFileNo; } 

private:
    static SInt   compare( const void *arg1,  const void *arg2 );

    // Read Info를 초기화 한다.
    IDE_RC initializeReadInfo( smiReadInfo * aReadInfo );

    // Read Info를 파괴한다.
    IDE_RC destroyReadInfo( smiReadInfo * aReadInfo );
    
    
private:
    /* Read정보를 가지고 있다. */
    smiReadInfo  mReadInfo;
    /* smrRedoInfo의 mLogHead의 mLSN을 기준으로 smrRedoInfo를
       Sorting해서 가지고 있다.*/
    iduPriorityQueue mPQueueRedoInfo;
    /* 현재 Read를 수행중인 smiReadInfo */
    smiReadInfo* mCurReadInfoPtr;
    /* 로그파일을 미리 읽는 Thread이다. */
    smrPreReadLFileThread mPreReadLFThread;
    /* 로그파일을 미리 읽을 갯수이다. */
    UInt         mPreReadFileCnt;
    /* 마지막으로 읽었던 로그의 Sequence Number */
    smLSN        mLstReadLogLSN;

    /* PROJ-1915 off-line 로그 인지 Local 로그인지 */
    idBool       mIsRemoteLog;
    /* PROJ-1915 리모트 로그 메니져 */

    smrRemoteLogMgr mRemoteLogMgr;
    /* BUG-46944 압축된 로그파일의 경우 압축을 풀지않고 반환할지 여부 (압축된 형태로 반환)*/
    idBool       mNotDecomp;
};

#endif /* _O_SMR_READ_LOG_BYORDER_H_ */
